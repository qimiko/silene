#pragma once

#ifndef _LIBC_ERRNO_H
#define _LIBC_ERRNO_H

#include "../environment.h"

std::uint32_t emu___errno(Environment& env) {
	spdlog::info("TODO: errno = {}", errno);
	return env.libc().get_errno_addr();
}

#endif