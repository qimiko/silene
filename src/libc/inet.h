#pragma once

#ifndef _LIBC_INET_H
#define _LIBC_INET_H

#include "../environment.h"

#include <arpa/inet.h>

std::int32_t emu_inet_pton(Environment& env, std::int32_t af, std::uint32_t src_ptr, std::uint32_t dst_ptr) {
	// is some sort of translation necessary?
	auto src = env.memory_manager().read_bytes<char>(src_ptr);
	auto dst = env.memory_manager().read_bytes<void>(dst_ptr);
	return inet_pton(af, src, dst);
}

#endif