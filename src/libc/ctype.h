#pragma once

#ifndef _LIBC_CTYPE_H
#define _LIBC_CTYPE_H

#include <cctype>

#include "../environment.h"

std::int32_t emu_tolower(Environment& env, std::int32_t ch) {
	return std::tolower(ch);
}

#endif
