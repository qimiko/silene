#pragma once

#ifndef _LIBC_POLL_H
#define _LIBC_POLL_H

#include "../environment.h"

#include <poll.h>

struct emu_pollfd {
	std::int32_t fd;
	std::int16_t events;
	std::int16_t revents;
};

std::int32_t emu_poll(Environment& env, std::uint32_t fds_ptr, std::uint32_t nfds, std::int32_t timeout) {
	auto emu_fds = env.memory_manager().read_bytes<emu_pollfd>(fds_ptr);

	std::vector<pollfd> fds{};
	fds.reserve(nfds);

	for (const auto& emu_fd : std::span{emu_fds, nfds}) {
		struct pollfd fd{
			.fd = emu_fd.fd,
			.events = emu_fd.events,
			.revents = emu_fd.revents
		};

		fds.push_back(fd);
	}

	auto r = poll(fds.data(), nfds, timeout);
	if (r == -1) {
		return -1;
	}

	for (auto i = 0u; i < nfds; i++) {
		auto fd = fds[i];
		emu_fds[i].revents = fd.revents;
	}

	return r;
}

#endif