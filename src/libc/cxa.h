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
	spdlog::info("TODO: __gnu_Unwind_Find_exidx");
	// TODO: return address of ARM.exidx section for c++ exception handling
	// see https://bugzilla.mozilla.org/show_bug.cgi?id=930627 for example
	// returning 0 should be safe?
	return 0;
}

void emu___stack_chk_fail(Environment& env) {
	throw new std::runtime_error("stack overflow detected");
}

#endif
