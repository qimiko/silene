#pragma once

#ifndef _LIBC_LOG_H
#define _LIBC_LOG_H

#include "../environment.h"

#include "stdio.h"

std::uint32_t emu___android_log_print(Environment& env, std::int32_t priority, std::uint32_t tag_ptr, std::uint32_t fmt_ptr, Variadic v) {
	auto tag = env.memory_manager().read_bytes<char>(tag_ptr);
	auto fmt = env.memory_manager().read_bytes<char>(fmt_ptr);

	auto formatted = perform_printf(env, fmt, v);
	spdlog::info("[emu::{}] {}", tag, formatted);

	return 1;
}

#endif

