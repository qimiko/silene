#pragma once

#ifndef _LIBC_SETJMP_H
#define _LIBC_SETJMP_H

#include "../environment.h"

// for whenever i want to implement these:
// https://android.googlesource.com/platform/bionic/+/refs/heads/main/libc/arch-arm/bionic/setjmp.S

std::uint32_t emu_setjmp(Environment& env) {
	spdlog::info("TODO: setjmp");
	return 0;
}

void emu_longjmp(Environment& env) {
	throw std::runtime_error("longjmp called! a program exception is happening (probably)");
	return;
}

#endif
