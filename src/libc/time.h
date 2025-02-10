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
			auto emu_tv = env.memory_manager().read_bytes<std::int32_t>(tv_ptr);
			emu_tv[0] = static_cast<std::int32_t>(tv.tv_sec);
			emu_tv[1] = static_cast<std::int32_t>(tv.tv_usec);
	}

	if (tz_ptr != 0) {
			auto emu_tz = env.memory_manager().read_bytes<std::int32_t>(tz_ptr);
			emu_tz[0] = static_cast<std::int32_t>(tz.tz_minuteswest);
			emu_tz[1] = static_cast<std::int32_t>(tz.tz_dsttime);
	}

	return 0;
}

std::int32_t emu_time(Environment& env, std::uint32_t arg_ptr) {
	auto r = static_cast<std::int32_t>(std::time(nullptr));

	if (arg_ptr != 0 && r != -1) {
		env.memory_manager().write_word(arg_ptr, r);
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

	env.memory_manager().write_word(tp_ptr, timeb_time); // timeb.time
	env.memory_manager().write_halfword(tp_ptr + 4, ms_tm); // timeb.millitm
	env.memory_manager().write_word(tp_ptr + 6, 0); // set timezone/dst to zero
	return 0;
}

int emu_clock_gettime(Environment& env, std::int32_t clock_id, std::uint32_t ts_ptr) {
	struct timespec ts{};

	auto r = clock_gettime(static_cast<clockid_t>(clock_id), &ts);
	if (r < 0) {
		return r;
	}

	env.memory_manager().write_word(ts_ptr, static_cast<std::int32_t>(ts.tv_sec));
	env.memory_manager().write_word(ts_ptr + 4, static_cast<std::int32_t>(ts.tv_nsec));

	return 0;
}

struct emu_tm {
  std::int32_t tm_sec;
  std::int32_t tm_min;
  std::int32_t tm_hour;
  std::int32_t tm_mday;
  std::int32_t tm_mon;
  std::int32_t tm_year;
  std::int32_t tm_wday;
  std::int32_t tm_yday;
  std::int32_t tm_isdst;
  std::int32_t tm_gmtoff;
  std::uint32_t tm_zone;
};

std::uint32_t emu_localtime(Environment& env, std::uint32_t time_ptr) {
	if (time_ptr == 0) {
		return 0;
	}

	struct std::tm lt{};
	std::time_t time = env.memory_manager().read_word(time_ptr);

	if (localtime_r(&time, &lt) == nullptr) {
		return 0;
	}

	auto zone_vaddr = 0u;
	if (lt.tm_zone != nullptr) {
		auto zone_size = strlen(lt.tm_zone);
		zone_vaddr = env.libc().allocate_memory(zone_size, false);
		env.memory_manager().copy(zone_vaddr, lt.tm_zone, zone_size + 1);
	}

	emu_tm et {
		.tm_sec=lt.tm_sec,
		.tm_min=lt.tm_min,
		.tm_hour=lt.tm_hour,
		.tm_mday=lt.tm_mday,
		.tm_mon=lt.tm_mon,
		.tm_year=lt.tm_year,
		.tm_wday=lt.tm_wday,
		.tm_yday=lt.tm_yday,
		.tm_isdst=lt.tm_isdst,
		.tm_gmtoff=static_cast<std::int32_t>(lt.tm_gmtoff),
		.tm_zone=zone_vaddr
	};

	auto r = env.libc().allocate_memory(sizeof(et), false);
	env.memory_manager().copy(r, &et, sizeof(et));

	return r;
}

#endif
