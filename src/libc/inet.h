#pragma once

#ifndef _LIBC_INET_H
#define _LIBC_INET_H

#include "../environment.h"

#include <arpa/inet.h>

std::int32_t emu_inet_pton(Environment& env, std::int32_t af, std::uint32_t src_ptr, std::uint32_t dst_ptr) {
#ifdef __APPLE__
	if (af == 10) {
		af = AF_INET6;
	}
#endif

	// is some sort of translation necessary?
	auto src = env.memory_manager().read_bytes<char>(src_ptr);
	auto dst = env.memory_manager().read_bytes<void>(dst_ptr);
	return inet_pton(af, src, dst);
}

std::uint32_t emu_inet_ntop(Environment& env, std::int32_t af, std::uint32_t src_ptr, std::uint32_t dst_ptr, std::int32_t size) {
#ifdef __APPLE__
	if (af == 10) {
		af = AF_INET6;
	}
#endif

	auto src = env.memory_manager().read_bytes<void>(src_ptr);
	auto dst = env.memory_manager().read_bytes<char>(dst_ptr);

	if (inet_ntop(af, src, dst, size) == nullptr) {
		return 0;
	}

	return dst_ptr;
}

#endif