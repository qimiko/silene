#include "android-environment.hpp"

void AndroidEnvironment::CallSVC(std::uint32_t swi) {
	spdlog::debug("svc call: {}", swi);

	switch (swi) {
		case 1:
			this->_cpu->HaltExecution(HALT_REASON_FN_END);
			break;
		case 2:
			// halt the cpu before doing syscalls
			// some functions require calling other functions, which cannot be done in the middle of a callback
			this->_cpu->HaltExecution(HALT_REASON_HANDLE_SYSCALL);
			break;
	}
}

void AndroidEnvironment::ExceptionRaised(std::uint32_t pc, Dynarmic::A32::Exception exception) {
	// todo: we should be able to backtrace?
	switch (exception) {
		case Dynarmic::A32::Exception::UnpredictableInstruction:
			spdlog::error("unpredictable instruction exception: pc = {:#08x}", pc);
			spdlog::info("value at {:#08x}: {:#08x}", pc, MemoryRead32(pc));
			this->_cpu->HaltExecution(HALT_REASON_ERROR);

			if (this->_debug_server) {
				this->_debug_server->report_halt(GdbServer::HaltReason::EmulationTrap);
			}
			break;
		case Dynarmic::A32::Exception::UndefinedInstruction:
			spdlog::error("undefined instruction exception: pc = {:#08x}", pc);
			spdlog::info("value at {:#08x}: {:#08x}", pc, MemoryRead32(pc));

			this->_cpu->HaltExecution(HALT_REASON_ERROR);
			if (this->_debug_server) {
				this->_debug_server->report_halt(GdbServer::HaltReason::IllegalInstruction);
			}
			break;
		default:
			spdlog::error("unknown exception {} raised: pc = {:#08x}", static_cast<std::uint32_t>(exception), pc);
			break;
	}

	ticks_left = 0;
}

void AndroidEnvironment::pre_init() {
	this->_memory->allocate_stack();

	// allocate enough memory for return functions
	this->_memory->allocate(0xFF);

	// currently nullptr should be null
	// maybe later add proper null page handling?

	// it should halt the cpu here
	this->MemoryWrite16(0x10, 0xdf01); // svc #0x1

	// fallback symbol handler
	this->MemoryWrite16(0x20, 0xdf02); // svc #0x2
	this->MemoryWrite16(0x22, 0x4770); // bx lr
	this->_program_loader.set_symbol_fallback_addr(0x21);

	this->_libc.pre_init(*this);
	this->_jni.pre_init(*this);
}

void AndroidEnvironment::post_init() {
	auto init_fns = this->_program_loader.get_init_functions();

	for (const auto& fn : init_fns) {
		if (fn == 0 || fn == -1) {
			continue;
		}

		this->run_func(fn);
	}
}

void AndroidEnvironment::run_func(std::uint32_t vaddr) {
	// enable thumb mode if lsb is set
	if (vaddr & 0x1) {
		_cpu->SetCpsr(0x00000030);
	} else {
		_cpu->SetCpsr(0x00000000);
	}

	auto fn_addr = vaddr & ~0x1;

	spdlog::info("calling fn: {:#08x}", fn_addr);

	// stack ptr, return ptr, pc
	this->_cpu->Regs()[13] = 0xffff'ffff;
	this->_cpu->Regs()[14] = 0x11;

	// strip the thumb bit
	this->_cpu->Regs()[15] = fn_addr;

	// track this for debugging idk
	auto last_return = 0u;

	while (1) {
		ticks_left = 10;

		// give an invalid value by default
		auto halt_reason = static_cast<Dynarmic::HaltReason>(0);
		if (this->_debug_server) {
			auto regs = _cpu->Regs();
			auto cpsr = _cpu->Cpsr();

			this->_debug_server->handle_events();
			halt_reason = this->_cpu->Step();
		} else {
			halt_reason = this->_cpu->Run();
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_HANDLE_SYSCALL)) {
			this->_syscall_handler.on_symbol_call(*this);
			halt_reason &= ~HALT_REASON_HANDLE_SYSCALL;
		}

		// 0 means it ran out of steps
		if (!halt_reason) {
			this->ticks_left += 10000;
			continue;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_ERROR)) {
			auto regs = _cpu->Regs();
			auto pc = regs[15];
			spdlog::warn("error at addr {:#08x}: {:#08x}", pc, _memory->read_word(pc));

			// at this point, the user may still want to continue debugging
			// so let the cpu continue
			if (this->_debug_server) {
				continue;
			}
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

void AndroidEnvironment::begin_debugging() {
	auto port = 5039;

	this->_debug_server = std::make_unique<GdbServer>(this->_memory, this->_cpu);
	this->_debug_server->begin_connection("0.0.0.0", port);
}
