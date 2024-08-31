#include "syscall-handler.hpp"

void SyscallHandler::on_symbol_call(Environment& env) {
	// resolve symbol
	auto pc = env.current_cpu()->Regs()[15];
	auto lr = env.current_cpu()->Regs()[14];
	spdlog::trace("resolve symbol: pc = {:#08x}, lr = {:#08x}", pc, lr);

	try {
		auto fn_ptr = this->fns.at(pc);
		fn_ptr(env);
	} catch (const std::out_of_range& e) {
		spdlog::warn("symbol resolution failed: lr = {:#08x}", lr);
	}
}

void SyscallHandler::on_kernel_call(Environment& env) {
	auto call_number = env.current_cpu()->Regs()[7];
	spdlog::trace("resolve kernel: call = {:#08x}", call_number);

	try {
		auto fn_ptr = this->_kernel_fns.at(call_number);
		fn_ptr(env);
	} catch (const std::out_of_range& e) {
		auto pc = env.current_cpu()->Regs()[15];
		spdlog::warn("failed to resolve kernel call: pc = {:#08x}, call = {:#x}", pc, call_number);
	}
}

void SyscallHandler::register_kernel_fn(std::uint32_t call, HandlerFunction fn) {
	this->_kernel_fns[call] = fn;
}

std::uint32_t SyscallHandler::create_stub_fn(HandlerFunction fn) {
	auto write_addr = this->_memory->get_next_addr();

	if (write_addr & 1) {
		// if the returned pointer is thumbed, increment by 1 to remove it
		write_addr++;
		this->_memory->allocate(1);
	}

	// allocate space for a double
	this->_memory->allocate(4);
	this->_memory->write_halfword(write_addr, 0xdf02);
	this->_memory->write_halfword(write_addr + 2, 0x4770);

	// thumb bit!
	this->fns[write_addr + 2] = fn;

	return write_addr | 1;
}

std::uint32_t SyscallHandler::replace_fn(std::uint32_t addr, HandlerFunction fn) {
	if (addr & 1) {
		// write thumb stub
		addr--;

		this->_memory->write_halfword(addr, 0xdf02);
		this->_memory->write_halfword(addr + 2, 0x4770);

		// thumb bit!
		this->fns[addr + 2] = fn;

		return addr | 1;
	}

	// non thumb
	this->_memory->write_word(addr, 0xef000002);
	this->_memory->write_word(addr, 0xe12fff1e);
	this->fns[addr + 4] = fn;

	return addr;
}
