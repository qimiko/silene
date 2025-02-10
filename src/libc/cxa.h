#pragma once

#ifndef _LIBC_CXA_H
#define _LIBC_CXA_H

#include "../environment.h"

int emu___cxa_atexit(Environment& env, std::uint32_t exit_fn_ptr, std::uint32_t fn_arg, std::uint32_t library_handle) {
	// there's not much use for this as destructors are currently never called
	// but it's easy to implement and keeps logs clean
	env.libc().register_destructor({ exit_fn_ptr, fn_arg });
	return 0;
}

void emu___cxa_finalize(Environment& env, std::uint32_t obj) {
	spdlog::info("TODO: __cxa_finalize");
	// nothing.
};

uint32_t emu___gnu_Unwind_Find_exidx(Environment& env, uint32_t pc, uint32_t pcount_ptr) {
	auto [begin, pcount] = env.program_loader().find_exidx(pc);

	if (pcount_ptr != 0) {
		env.memory_manager().write_word(pcount_ptr, pcount);
	}

	return begin;
}

void emu___stack_chk_fail(Environment& env) {
	throw new std::runtime_error("stack overflow detected");
}

#endif
