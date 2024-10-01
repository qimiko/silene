#pragma once

#ifndef _LIBC_SYS_TIME_H
#define _LIBC_SYS_TIME_H

#include <sys/time.h>

#include "../environment.h"

int emu_gettimeofday(Environment& env, std::uint32_t tv_ptr, std::uint32_t tz_ptr) {
	struct timeval tv{};
	struct timezone tz{};

	auto r = gettimeofday(&tv, &tz);

	if (r != 0) {
		return r;
	}

	if (tv_ptr != 0) {
			auto emu_tv = env.memory_manager()->read_bytes<std::int32_t>(tv_ptr);
			emu_tv[0] = static_cast<std::int32_t>(tv.tv_sec);
			emu_tv[1] = static_cast<std::int32_t>(tv.tv_usec);
	}

	if (tz_ptr != 0) {
			auto emu_tz = env.memory_manager()->read_bytes<std::int32_t>(tz_ptr);
			emu_tz[0] = static_cast<std::int32_t>(tz.tz_minuteswest);
			emu_tz[1] = static_cast<std::int32_t>(tz.tz_dsttime);
	}

	return 0;
}

std::int32_t emu_time(Environment& env, std::uint32_t arg_ptr) {
	auto r = static_cast<std::int32_t>(std::time(nullptr));

	if (arg_ptr != 0 && r != -1) {
		env.memory_manager()->write_word(arg_ptr, r);
	}

	return r;
}

int emu_ftime(Environment& env, std::uint32_t tp_ptr) {
	struct timespec ts{};
	if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
		return -1;
	}

	auto timeb_time = static_cast<std::int32_t>(ts.tv_sec);
	std::uint16_t ms_tm = (ts.tv_nsec + 500000) / 1000000;

	if (ms_tm == 1000) {
		timeb_time++;
		ms_tm = 0;
	}

	env.memory_manager()->write_word(tp_ptr, timeb_time); // timeb.time
	env.memory_manager()->write_halfword(tp_ptr + 4, ms_tm); // timeb.millitm
	env.memory_manager()->write_word(tp_ptr + 6, 0); // set timezone/dst to zero
	return 0;
}

#endif
