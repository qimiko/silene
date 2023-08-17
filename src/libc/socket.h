#pragma once

#ifndef _LIBC_SOCKET_H
#define _LIBC_SOCKET_H

#include "../environment.h"

std::int32_t emu_getsockopt(Environment& env, std::int32_t socket, std::int32_t level, std::int32_t option_name, std::uint32_t option_value_ptr, std::uint32_t option_len_ptr) {
	spdlog::info("TODO: getsockopt", socket, level, option_name);
	return -1;
}


#endif