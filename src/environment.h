#pragma once

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <dynarmic/interface/A32/a32.h>

#include "paged-memory.hpp"
#include "elf-loader.h"

// i tried to avoid making this a circular reference for so long
class LibcState;
class SyscallHandler;
class JniState;

/**
 * abstract class to hold global state
 * prevents circular dependency with syscall handler
 * should be part of AndroidEnvironment.
*/
class Environment {
public:
	virtual std::shared_ptr<PagedMemory> memory_manager() = 0;
	virtual std::shared_ptr<Dynarmic::A32::Jit> current_cpu() = 0;
	virtual LibcState& libc() = 0;
	virtual JniState& jni() = 0;
	virtual SyscallHandler& syscall_handler() = 0;
	virtual Elf::Loader& program_loader() = 0;

	/**
	 * outputs the current state of the environment
	 */
	virtual void dump_state() = 0;

	virtual std::string assets_dir() = 0;

	/**
	 * run a function at some address, assuming that all arguments are already setup
	 */
	virtual void run_func(std::uint32_t vaddr) = 0;
};

#endif