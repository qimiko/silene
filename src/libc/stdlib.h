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
	auto begin = env.memory_manager().read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtol(begin, &end, base);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager().write_word(end_ptr, emu_end);
	}

	return x;
}

std::int64_t emu_strtoll(Environment& env, std::uint32_t begin_ptr, std::uint32_t end_ptr, std::int32_t base) {
	auto begin = env.memory_manager().read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtoll(begin, &end, base);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager().write_word(end_ptr, emu_end);
	}

	return x;
}

std::uint32_t emu_strtoul(Environment& env, std::uint32_t begin_ptr, std::uint32_t end_ptr, std::int32_t base) {
	auto begin = env.memory_manager().read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtoul(begin, &end, base);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager().write_word(end_ptr, emu_end);
	}

	return x;
}

double emu_strtod(Environment& env, std::uint32_t begin_ptr, std::uint32_t end_ptr) {
	auto begin = env.memory_manager().read_bytes<char>(begin_ptr);

	char* end;
	auto x = std::strtod(begin, &end);

	if (end_ptr != 0) {
		auto offs = end - begin;
		auto emu_end = begin_ptr + offs;

		env.memory_manager().write_word(end_ptr, emu_end);
	}

	return x;
}

int emu_atoi(Environment& env, std::uint32_t str_ptr) {
	auto str = env.memory_manager().read_bytes<char>(str_ptr);
	return std::atoi(str);
}

double emu_atof(Environment& env, std::uint32_t str_ptr) {
	auto str = env.memory_manager().read_bytes<char>(str_ptr);
	return std::atof(str);
}

std::uint32_t emu_getenv(Environment& env, std::uint32_t name_ptr) {
	return 0;
}

std::int32_t emu_lrand48(Environment& env) {
	return static_cast<std::int32_t>(lrand48());
}

std::uint32_t emu_arc4random(Environment& env) {
	return arc4random();
}

void emu_srand48(Environment& env, std::int32_t seed) {
	srand48(seed);
}

struct qsort_context {
	Environment& env;
	std::uint32_t comp_fn;
	std::intptr_t emu_diff;
};

thread_local qsort_context* active_qsort = nullptr;

int qsort_inner(const void* a, const void* b) {
	if (active_qsort == nullptr) {
		spdlog::warn("qsort context is incorrectly nullptr");
		return 0;
	}

	auto context = reinterpret_cast<qsort_context*>(active_qsort);
	auto& env = context->env;
	auto& regs = env.current_cpu()->Regs();

	auto emu_a = static_cast<std::uint32_t>(reinterpret_cast<std::intptr_t>(a) - context->emu_diff);
	auto emu_b = static_cast<std::uint32_t>(reinterpret_cast<std::intptr_t>(b) - context->emu_diff);

	regs[0] = emu_a;
	regs[1] = emu_b;

	context->env.run_func(context->comp_fn);

	auto r = std::bit_cast<std::int32_t>(regs[0]);
	return r;
}

// ptr - base = a
// base + a = base

void emu_qsort(Environment& env, std::uint32_t ptr, std::uint32_t count, std::uint32_t size, std::uint32_t comp_fn) {
	// comp = (std::uint32_t a, std::uint32_t b) -> int

	if (count < 2) {
		return;
	}

	auto base = env.memory_manager().read_bytes<void>(ptr);
	qsort_context context = {
		.env = env,
		.comp_fn = comp_fn,
		.emu_diff = reinterpret_cast<std::intptr_t>(base) - ptr
	};

	active_qsort = &context;

	// we could use qsort_r, but it's not available on android until android 14
	// that's really odd considering its implementation was pulled from freebsd, but whatever
	qsort(base, count, size, &qsort_inner);

	active_qsort = nullptr;
}

#endif
