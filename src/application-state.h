#pragma once

#ifndef _APPLICATION_STATE_H
#define _APPLICATION_STATE_H

#include "paged-memory.hpp"
#include "elf-loader.h"
#include "syscall-handler.hpp"
#include "libc-state.h"
#include "jni.h"

struct ApplicationState {
	PagedMemory memory;
	Elf::Loader program_loader;
	SyscallHandler syscall_handler;
	LibcState libc;
	JniState jni;

	ApplicationState() :
		memory{}, program_loader{memory}, syscall_handler{memory}, libc{memory}, jni{memory} {}

	ApplicationState(const ApplicationState&) = delete;
	ApplicationState& operator=(const ApplicationState&) = delete;
};

class StateHolder {
private:
	ApplicationState& _state;

public:
	inline PagedMemory& memory_manager() const {
		return this->_state.memory;
	}

	inline Elf::Loader& program_loader() const {
		return this->_state.program_loader;
	}

	inline SyscallHandler& syscall_handler() const {
		return this->_state.syscall_handler;
	}

	inline LibcState& libc() const {
		return this->_state.libc;
	}

	inline JniState& jni() const  {
		return this->_state.jni;
	}

	StateHolder(ApplicationState& state) : _state{state} {}
};

#endif