#include "application-state.h"

PagedMemory& StateHolder::memory_manager() const {
	return this->_state.memory;
}

Elf::Loader& StateHolder::program_loader() const {
	return this->_state.program_loader;
}

SyscallHandler& StateHolder::syscall_handler() const {
	return this->_state.syscall_handler;
}

LibcState& StateHolder::libc() const {
	return this->_state.libc;
}

JniState& StateHolder::jni() const {
	return this->_state.jni;
}

StateHolder::StateHolder(ApplicationState& state) : _state{state} {}
