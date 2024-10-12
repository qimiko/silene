#pragma once

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <cstdint>
#include <memory>

#include <dynarmic/interface/A32/a32.h>

#include "application-state.h"

/**
 * abstract class to hold global state
 * prevents circular dependency with syscall handler
 * should be part of AndroidEnvironment.
*/
class Environment : public StateHolder {
public:
	virtual std::shared_ptr<Dynarmic::A32::Jit> current_cpu() = 0;

	/**
	 * outputs the current state of the environment
	 */
	virtual void dump_state() = 0;

	/**
	 * run a function at some address, assuming that all arguments are already setup
	 */
	virtual void run_func(std::uint32_t vaddr) = 0;

	Environment(ApplicationState& state) : StateHolder(state) {}
};

#endif