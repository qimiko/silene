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
	PagedMemory& memory_manager() const;
	Elf::Loader& program_loader() const;
	SyscallHandler& syscall_handler() const;
	LibcState& libc() const;
	JniState& jni() const;

	StateHolder(ApplicationState&);
};

#endif