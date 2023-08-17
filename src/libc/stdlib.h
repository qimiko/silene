#pragma once

#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include "../environment.h"

std::uint32_t emu_malloc(Environment& env, std::uint32_t size) {
	return env.libc().allocate_memory(size);
}

void emu_free(Environment& env, std::uint32_t ptr) {
	env.libc().free_memory(ptr);
	return;
}

std::uint32_t emu_realloc(Environment& env, std::uint32_t ptr, std::uint32_t new_size) {
	return env.libc().reallocate_memory(ptr, new_size);
}

std::uint32_t emu_calloc(Environment& env, std::uint32_t num, std::uint32_t size) {
	return env.libc().allocate_memory(num * size, true);
}

void emu_abort(Environment& env) {
	throw std::runtime_error("program abort called");
}

#endif