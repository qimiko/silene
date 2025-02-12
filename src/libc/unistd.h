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

std::int32_t emu_write(Environment& env, std::int32_t fd, std::uint32_t buf_ptr, std::uint32_t count) {
	auto buf = env.memory_manager().read_bytes<void>(buf_ptr);
	return write(fd, buf, count);
}

std::int32_t emu_read(Environment& env, std::int32_t fd, std::uint32_t buf_ptr, std::uint32_t count) {
	auto buf = env.memory_manager().read_bytes<void>(buf_ptr);
	return read(fd, buf, count);
}

std::int32_t emu_getpid(Environment& env) {
	return 2;
}

std::uint32_t emu_getuid(Environment& env) {
	return 1;
}

std::uint32_t emu_geteuid(Environment& env) {
	return 1;
}

std::uint32_t emu_getgid(Environment& env) {
	return 1;
}

std::uint32_t emu_getegid(Environment& env) {
	return 1;
}

#endif