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

std::int32_t emu_strtol(Environment& env, std::uint32_t begin_ptr, std::uint32_t end_ptr, std::int32_t base) {
	auto begin = env.memory_manager()->read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtol(begin, &end, base);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager()->write_word(end_ptr, emu_end);
	}

	return x;
}

float emu_strtod(Environment& env, std::uint32_t begin_ptr, std::uint32_t end_ptr) {
	auto begin = env.memory_manager()->read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtod(begin, &end);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager()->write_word(end_ptr, emu_end);
	}

	return x;
}

#endif