#include "syscall-handler.hpp"

void SyscallHandler::on_symbol_call(Environment& env) {
	// resolve symbol
	auto pc = env.current_cpu()->Regs()[15];
	auto lr = env.current_cpu()->Regs()[14];
	spdlog::debug("resolve symbol: pc = {:#08x}, lr = {:#08x}", pc, lr);

	try {
		// todo: literally anything
		auto fn_ptr = this->fns.at(pc);
		fn_ptr(env);
	} catch (const std::out_of_range& e) {
		spdlog::warn("symbol resolution failed: lr = {:#08x}", lr);
	}
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
