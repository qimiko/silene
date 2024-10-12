#pragma once

#ifndef _LIBC_WCTYPE_H
#define _LIBC_WCTYPE_H

#include <cwctype>

#include "../environment.h"

std::uint32_t emu_wctype(Environment& env, std::uint32_t str_ptr) {
	auto str = env.memory_manager().read_bytes<char>(str_ptr);

	return std::wctype(str);
}

#endif
