#pragma once

#ifndef _LIBC_SETJMP_H
#define _LIBC_SETJMP_H

#include "../environment.h"

// for whenever i want to implement these:
// https://android.googlesource.com/platform/bionic/+/refs/heads/main/libc/arch-arm/bionic/setjmp.S

std::int32_t emu_setjmp(Environment& env, std::uint32_t buf) {
	spdlog::info("TODO: setjmp({:#x})", buf);
	return 0;
}

void emu_longjmp(Environment& env, std::uint32_t buf, std::int32_t status) {
	throw std::runtime_error("longjmp called! a program exception is happening (probably)");
	return;
}

std::int32_t emu_sigsetjmp(Environment& env, std::uint32_t buf, std::int32_t savesigs) {
	spdlog::info("TODO: sigsetjmp({:#x}, {})", buf, savesigs);
	return 0;
}

#endif
