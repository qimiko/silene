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
			throw std::runtime_error("unpredictable instruction");
			break;
		case Dynarmic::A32::Exception::UndefinedInstruction:
			spdlog::error("undefined instruction exception: pc = {:#08x}", pc);
			spdlog::info("value at {:#08x}: {:#08x}", pc, MemoryRead32(pc));
			this->_cpu->HaltExecution(HALT_REASON_ERROR);
			throw std::runtime_error("undefined instruction");
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

		auto halt_reason = this->_cpu->Step();

		auto regs = _cpu->Regs();
		spdlog::debug("step at addr {:#08x} ({:#08x}):\n{:#08x} {:#08x} {:#08x} {:#08x} {:#08x}\n{:#08x} {:#08x} {:#08x} {:#08x} {:#08x}\n{:#08x} {:#08x} {:#08x} {:#08x} {:#08x}",
			regs[15], this->_memory->read_word(regs[15]), regs[0], regs[1], regs[2], regs[3], regs[4],
			regs[5], regs[6], regs[7], regs[8], regs[9],
			regs[10], regs[11], regs[12], regs[13], regs[14]);
		spdlog::debug("cpu halted, reason {:#x}", static_cast<std::uint32_t>(halt_reason));

		if (last_return != regs[14]) {
			last_return = regs[14];
			spdlog::debug("jump detected! lr = {:#08x}, pc = {:#08x}", last_return, regs[15]);
		}

		// 0 means it ran out of steps
		if (!halt_reason) {
			this->ticks_left += 10000;
			continue;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_HANDLE_SYSCALL)) {
			this->_syscall_handler.on_symbol_call(*this);
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_ERROR)) {
			auto regs = _cpu->Regs();
			auto pc = regs[15];
			spdlog::warn("error at addr {:#08x}: {:#08x}", pc, _memory->read_word(pc));
			return;
		}

		if (Dynarmic::Has(halt_reason, HALT_REASON_FN_END)) {
			return;
		}

		if (Dynarmic::Has(halt_reason, Dynarmic::HaltReason::Step)) {
			continue;
		}

		throw std::runtime_error("unexpected cpu halt");
	}
}
