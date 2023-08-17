#pragma once

#ifndef _LIBC_LOCALE_H
#define _LIBC_LOCALE_H

#include "../environment.h"

std::uint32_t emu_setlocale(Environment& env, std::int32_t category, std::uint32_t locale_ptr) {
	spdlog::info("TODO: setlocale");

	// should copy this memory..?
	return locale_ptr;
}

#endif