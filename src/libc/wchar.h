#pragma once

#ifndef _LIBC_WCHAR_H
#define _LIBC_WCHAR_H

#include <cwchar>

#include "../environment.h"

// what is a wint_t?

std::uint32_t emu_btowc(Environment& env, std::uint32_t c) {
	return std::btowc(c);
}

std::int32_t emu_wctob(Environment& env, std::uint32_t c) {
	return std::wctob(c);
}

std::uint32_t emu_wcslen(Environment& env, std::uint32_t str_ptr) {
	auto str = env.memory_manager().read_bytes<wchar_t>(str_ptr);

	return std::wcslen(str);
}

#endif