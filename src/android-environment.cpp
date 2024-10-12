#include "android-environment.hpp"

std::uint8_t AndroidEnvironment::MemoryRead8(std::uint32_t vaddr) {
	return this->memory_manager().read_byte(vaddr);
}

std::uint16_t AndroidEnvironment::MemoryRead16(std::uint32_t vaddr) {
	return this->memory_manager().read_halfword(vaddr);
}

std::uint32_t AndroidEnvironment::MemoryRead32(std::uint32_t vaddr) {
	if (vaddr == 0x0) {
		spdlog::warn("careful... null read");
		this->dump_state();

		if (this->_debug_server) {
			this->_debug_server->report_halt(GdbServer::HaltReason::SegmentationFault);
		}
	}

	return this->memory_manager().read_word(vaddr);
}

std::uint64_t AndroidEnvironment::MemoryRead64(std::uint32_t vaddr) {
	return this->memory_manager().read_doubleword(vaddr);
}

void AndroidEnvironment::MemoryWrite8(std::uint32_t vaddr, std::uint8_t value) {
	this->memory_manager().write_byte(vaddr, value);
}

void AndroidEnvironment::MemoryWrite16(std::uint32_t vaddr, std::uint16_t value) {
	this->memory_manager().write_halfword(vaddr, value);
}

void AndroidEnvironment::MemoryWrite32(std::uint32_t vaddr, std::uint32_t value) {
	this->memory_manager().write_word(vaddr, value);
}

void AndroidEnvironment::MemoryWrite64(std::uint32_t vaddr, std::uint64_t value) {
	this->memory_manager().write_doubleword(vaddr, value);
}

// todo: obviously this won't work once threads are introduced
// figure things out when that happens
bool AndroidEnvironment::MemoryWriteExclusive8(std::uint32_t vaddr, std::uint8_t value, std::uint8_t expected) {
	this->memory_manager().write_byte(vaddr, value);
	return true;
}

bool AndroidEnvironment::MemoryWriteExclusive16(std::uint32_t vaddr, std::uint16_t value, std::uint16_t expected) {
	this->memory_manager().write_halfword(vaddr, value);
	return true;
}

bool AndroidEnvironment::MemoryWriteExclusive32(std::uint32_t vaddr, std::uint32_t value, std::uint32_t expected) {
	this->memory_manager().write_word(vaddr, value);
	return true;
}

bool AndroidEnvironment::MemoryWriteExclusive64(std::uint32_t vaddr, std::uint64_t value, std::uint64_t expected) {
	this->memory_manager().write_doubleword(vaddr, value);
	return true;
}

void AndroidEnvironment::CallSVC(std::uint32_t swi) {
	spdlog::trace("svc call: {}", swi);

	switch (swi) {
		case 0:
			this->_cpu->HaltExecution(HALT_REASON_KERNEL_SYSCALL);
			break;
		case 1:
			this->_cpu->HaltExecution(HALT_REASON_FN_END);
			break;
		case 2:
			// halt the cpu before doing syscalls
			// some functions require calling other functions, which cannot be done in the middle of a callback
			this->_cpu->HaltExecution(HALT_REASON_HANDLE_SYSCALL);
			break;
		default:
			spdlog::error("thrown unexpected syscall of {}", swi);
			throw std::runtime_error("unexpected svc call");
	}
}

void AndroidEnvironment::ExceptionRaised(std::uint32_t pc, Dynarmic::A32::Exception exception) {
	using namespace Dynarmic::A32;

	// todo: we should be able to backtrace?
	switch (exception) {
		case Exception::UnpredictableInstruction:
			spdlog::error("unpredictable instruction exception: pc = {:#08x}", pc);
			spdlog::info("value at {:#08x}: {:#08x}", pc, MemoryRead32(pc));
			this->_cpu->HaltExecution(HALT_REASON_ERROR);

			if (this->_debug_server) {
				this->_debug_server->report_halt(GdbServer::HaltReason::EmulationTrap);
			}
			break;
		case Exception::UndefinedInstruction:
			spdlog::error("undefined instruction exception: pc = {:#08x}", pc);
			spdlog::info("value at {:#08x}: {:#08x}", pc, MemoryRead32(pc));

			this->_cpu->HaltExecution(HALT_REASON_ERROR);
			if (this->_debug_server) {
				this->_debug_server->report_halt(GdbServer::HaltReason::IllegalInstruction);
			}
			break;
		case Exception::Breakpoint:
			spdlog::info("breakpoint hit: pc = {:#08x}", pc);

			this->_cpu->HaltExecution(static_cast<Dynarmic::HaltReason>(0));
			if (this->_debug_server) {
				this->_debug_server->report_halt(GdbServer::HaltReason::SwBreak);
			}
			break;
		default:
			spdlog::error("unknown exception {} raised: pc = {:#08x}", static_cast<std::uint32_t>(exception), pc);
			break;
	}

	ticks_left = 0;
}

void AndroidEnvironment::run_func(std::uint32_t vaddr) {
	// enable thumb mode if lsb is set
	if (vaddr & 0x1) {
		_cpu->SetCpsr(0x00000030);
	} else {
		_cpu->SetCpsr(0x00000000);
	}

	auto fn_addr = vaddr & ~0x1;

	spdlog::trace("calling fn: {:#08x}", fn_addr);

	// stack ptr, return ptr, pc
	this->_cpu->Regs()[13] = 0xffff'fff0;
	this->_cpu->Regs()[14] = 0x11;

	// strip the thumb bit
	this->_cpu->Regs()[15] = fn_addr;

	while (1) {
		ticks_left = 10;

		// give an invalid value by default
		auto halt_reason = static_cast<Dynarmic::HaltReason>(0);
		if (this->_debug_server) {
			this->_debug_server->handle_events();
			halt_reason = this->_cpu->Step();
		} else {
			halt_reason = this->_cpu->Run();
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_HANDLE_SYSCALL)) {
			try {
				this->syscall_handler().on_symbol_call(*this);
			} catch (...) {
				spdlog::error("unhandled exception in symbol handler");
				this->dump_state();

				throw;
			}
			halt_reason &= ~HALT_REASON_HANDLE_SYSCALL;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_KERNEL_SYSCALL)) {
			this->syscall_handler().on_kernel_call(*this);
			halt_reason &= ~HALT_REASON_KERNEL_SYSCALL;
		}

		// 0 means it ran out of steps
		if (!halt_reason) {
			this->ticks_left += 50000;
			continue;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_ERROR)) {
			auto regs = _cpu->Regs();
			auto pc = regs[15];
			spdlog::warn("error at addr {:#08x}: {:#08x}", pc, this->memory_manager().read_word(pc));

			// at this point, the user may still want to continue debugging
			// so let the cpu continue
			if (this->_debug_server) {
				continue;
			}

			this->dump_state();
			return;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_FN_END)) {
			return;
		}

		if (Dynarmic::Has(halt_reason, Dynarmic::HaltReason::Step)) {
			continue;
		}

		spdlog::error("received unexpected halt: {:#x}", static_cast<int>(halt_reason));
		throw std::runtime_error("unexpected cpu halt");
	}
}

void AndroidEnvironment::dump_state() {
	auto& regs = _cpu->Regs();
	spdlog::info("register states:\nr0:  {:#010x} r1:  {:#010x} r2:  {:#010x} r3:  {:#010x}\nr4:  {:#010x} r5:  {:#010x} r6:  {:#010x} r7:  {:#010x}\nr8:  {:#010x} r9:  {:#010x} r10: {:#010x} r11: {:#010x}\nr12: {:#010x} sp:  {:#010x} lr:  {:#010x} pc:  {:#010x}",
		regs[0], regs[1], regs[2], regs[3],
		regs[4], regs[5], regs[6], regs[7],
		regs[8], regs[9], regs[10], regs[11],
		regs[12], regs[13], regs[14], regs[15]
	);
}

void AndroidEnvironment::begin_debugging() {
	auto port = 5039;

	this->_debug_server = std::make_unique<GdbServer>(this->memory_manager(), this->_cpu);
	this->_debug_server->begin_connection("0.0.0.0", port);
}

AndroidEnvironment::AndroidEnvironment(ApplicationState& state) : Environment(state) {}
