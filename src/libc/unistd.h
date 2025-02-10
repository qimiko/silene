#pragma once

#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H

#include "../environment.h"

#include <unistd.h>

std::int32_t emu_close(Environment& env, std::int32_t fd) {
	return close(fd);
}

std::uint32_t emu_alarm(Environment& env, std::uint32_t seconds) {
	spdlog::info("TODO: alarm({})", seconds);
	return 0;
}

#endif