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

#endif