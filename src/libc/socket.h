#pragma once

#ifndef _LIBC_SOCKET_H
#define _LIBC_SOCKET_H

#include "../environment.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

std::int32_t emu_getsockopt(Environment& env, std::int32_t socket, std::int32_t level, std::int32_t option_name, std::uint32_t option_value_ptr, std::uint32_t option_len_ptr) {
	spdlog::info("TODO: getsockopt({}, {}, {})", socket, level, option_name);

	// thanks macos
	switch (level) {
		case 1:
			level = SOL_SOCKET;
			break;
	}

	switch (option_name) {
		case 4:
			option_name = SO_ERROR;
			break;
	}

	auto option_value = env.memory_manager().read_bytes<void>(option_value_ptr);
	auto option_len = env.memory_manager().read_bytes<std::uint32_t>(option_len_ptr);

	return getsockopt(socket, level, option_name, option_value, option_len);
}

std::int32_t domain_to_system(std::int32_t domain) {
	#ifdef __APPLE__
		switch (domain) {
			case 10:
				return AF_INET6;
			default:
				return domain;
		}
	#else
		return domain;
	#endif
}

std::int32_t domain_to_emu(std::int32_t domain) {
	#ifdef __APPLE__
		switch (domain) {
			case AF_INET6:
				return 10;
			default:
				return domain;
		}
	#else
		return domain;
	#endif
}

std::int32_t emu_socket(Environment& env, std::int32_t domain, std::int32_t type, std::int32_t protocol) {
	// this is a bad idea isn't it
	return socket(domain_to_system(domain), type, protocol);
}

struct emu_sockaddr {
	std::uint16_t sa_family;
	std::int8_t sa_data[14];
};

struct emu_addrinfo {
	std::int32_t ai_flags;
	std::int32_t ai_family;
	std::int32_t ai_socktype;
	std::int32_t ai_protocol;
	std::int32_t ai_addrlen;
	std::uint32_t ai_canonname_ptr;
	std::uint32_t ai_addr_ptr;
	std::uint32_t ai_next_ptr;
};

std::int32_t emu_getaddrinfo(Environment& env, std::uint32_t node_ptr, std::uint32_t service_ptr, std::uint32_t hints_ptr, std::uint32_t res_ptr) {
	// good one
	char* node = nullptr;
	if (node_ptr != 0) {
		node = env.memory_manager().read_bytes<char>(node_ptr);
	}

	char* service = nullptr;
	if (service_ptr != 0) {
		service = env.memory_manager().read_bytes<char>(service_ptr);
	}

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));

	if (hints_ptr != 0) {
		auto emu_hints = env.memory_manager().read_bytes<emu_addrinfo>(hints_ptr);
		hints.ai_socktype = emu_hints->ai_socktype;
		hints.ai_protocol = emu_hints->ai_protocol;
		hints.ai_family = emu_hints->ai_family;
		hints.ai_flags = emu_hints->ai_flags;
	} else {
		hints.ai_socktype = 0;
		hints.ai_protocol = 0;
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
	}

	struct addrinfo* result;

	if (auto status = getaddrinfo(node, service, &hints, &result); status != 0) {
		return status;
	}

	// we have to translate it yay !!!
	auto copy_addr_info = [](Environment& env, addrinfo* obj, auto& copy_impl) -> std::uint32_t {
		if (obj == nullptr) {
			return 0;
		}

		auto addr_mem = env.libc().allocate_memory(sizeof(emu_addrinfo));

		auto addr_ptr = 0u;
		if (obj->ai_addr != nullptr) {
			addr_ptr = env.libc().allocate_memory(obj->ai_addrlen);
			emu_sockaddr emu_addr;

			emu_addr.sa_family = domain_to_emu(obj->ai_addr->sa_family);
			memcpy(&emu_addr.sa_data, &obj->ai_addr->sa_data, 14);

			env.memory_manager().copy(addr_ptr, &emu_addr, sizeof(emu_sockaddr));
		}

		auto canonname_ptr = 0u;
		if (obj->ai_canonname != nullptr) {
			auto len = strlen(obj->ai_canonname) + 1;
			canonname_ptr = env.libc().allocate_memory(len);
			env.memory_manager().copy(canonname_ptr, obj->ai_canonname, len);
		}

		emu_addrinfo emu_obj = {
			.ai_flags = obj->ai_flags,
			.ai_family = domain_to_emu(obj->ai_family),
			.ai_socktype = obj->ai_socktype,
			.ai_protocol = obj->ai_protocol,
			.ai_addrlen = sizeof(emu_sockaddr),
			.ai_canonname_ptr = canonname_ptr,
			.ai_addr_ptr = addr_ptr,
			.ai_next_ptr = copy_impl(env, obj->ai_next, copy_impl)
		};

		env.memory_manager().copy(addr_mem, &emu_obj, sizeof(emu_addrinfo));

		return addr_mem;
	};

	auto r = copy_addr_info(env, result, copy_addr_info);

	freeaddrinfo(result);

	env.memory_manager().write_word(res_ptr, r);

	return 0;
}

void emu_freeaddrinfo(Environment& env, std::uint32_t addrinfo_ptr) {
	if (addrinfo_ptr == 0) {
		return;
	}

	auto free_addr_info = [](Environment& env, std::uint32_t obj, auto& free_impl) {
		if (obj == 0) {
			return;
		}

		auto addrinfo = env.memory_manager().read_bytes<emu_addrinfo>(obj);
		env.libc().free_memory(addrinfo->ai_addr_ptr);
		env.libc().free_memory(addrinfo->ai_canonname_ptr);

		free_impl(env, addrinfo->ai_next_ptr, free_impl);

		env.libc().free_memory(obj);
	};

	free_addr_info(env, addrinfo_ptr, free_addr_info);
}

std::int32_t emu_connect(Environment& env, std::int32_t sockfd, std::uint32_t addr_ptr, std::int32_t addrlen) {
	auto emu_addr = env.memory_manager().read_bytes<emu_sockaddr>(addr_ptr);
	struct sockaddr addr{};
#ifdef __APPLE__
	addr.sa_len = 0x10;
#endif
	addr.sa_family = emu_addr->sa_family;
	memcpy(&addr.sa_data, &emu_addr->sa_data, 14);

	return connect(sockfd, &addr, 16);
}

std::int32_t emu_getpeername(Environment& env, std::int32_t sockfd, std::uint32_t addr_ptr, std::uint32_t len_ptr) {
	auto emu_addr = env.memory_manager().read_bytes<emu_sockaddr>(addr_ptr);

	struct sockaddr addr{};
	socklen_t addrlen = sizeof(struct sockaddr);

	if (getpeername(sockfd, &addr, &addrlen) == -1) {
		return -1;
	}

	emu_addr->sa_family = domain_to_emu(addr.sa_family);
	memcpy(&emu_addr->sa_data, &addr.sa_data, 14);

	env.memory_manager().write_word(len_ptr, sizeof(emu_sockaddr));

	return 0;
}

std::int32_t emu_getsockname(Environment& env, std::int32_t sockfd, std::uint32_t addr_ptr, std::uint32_t len_ptr) {
	auto emu_addr = env.memory_manager().read_bytes<emu_sockaddr>(addr_ptr);

	struct sockaddr addr{};
	socklen_t addrlen;

	if (getsockname(sockfd, &addr, &addrlen) == -1) {
		return -1;
	}

	emu_addr->sa_family = domain_to_emu(addr.sa_family);
	memcpy(&emu_addr->sa_data, &addr.sa_data, 14);

	env.memory_manager().write_word(len_ptr, sizeof(emu_sockaddr));

	return 0;
}

std::int32_t emu_send(Environment& env, std::int32_t sockfd, std::uint32_t buf_ptr, std::uint32_t size, std::uint32_t flags) {
	if (flags & 0x4000) {
		flags = (flags & ~0x4000) | MSG_NOSIGNAL;
	}

	spdlog::info("TODO: send({}, {:#x}, {}, {:#x})", sockfd, buf_ptr, size, flags);

	auto buf = env.memory_manager().read_bytes<void>(buf_ptr);

	// spdlog::info("send: {}",reinterpret_cast<char*>(buf));

	return send(sockfd, buf, size, flags);
}

std::int32_t emu_recv(Environment& env, std::int32_t sockfd, std::uint32_t buf_ptr, std::uint32_t size, std::int32_t flags) {
	spdlog::info("TODO: recv({}, {:#x}, {}, {:#x})", sockfd, buf_ptr, size, flags);

	auto buf = env.memory_manager().read_bytes<void>(buf_ptr);
	auto r = recv(sockfd, buf, size, flags);

	// spdlog::info("recv: {}", std::string_view{reinterpret_cast<char*>(buf), 512});

	return r;
}

#endif