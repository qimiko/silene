#pragma once

#ifndef _LIBC_SIGNAL_H
#define _LIBC_SIGNAL_H

#include "../environment.h"

std::int32_t emu_sigaction(Environment& env, std::int32_t signum, std::uint32_t act_ptr, std::uint32_t oldact_ptr) {
	spdlog::info("TODO: sigaction(signum={})", signum);
	return 0;
}

#endif
