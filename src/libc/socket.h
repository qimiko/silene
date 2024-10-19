#pragma once

#ifndef _LIBC_SOCKET_H
#define _LIBC_SOCKET_H

#include "../environment.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

std::int32_t emu_getsockopt(Environment& env, std::int32_t socket, std::int32_t level, std::int32_t option_name, std::uint32_t option_value_ptr, std::uint32_t option_len_ptr) {
	spdlog::info("TODO: getsockopt({}, {}, {})", socket, level, option_name);
	return -1;
}

std::int32_t emu_socket(Environment& env, std::int32_t domain, std::int32_t type, std::int32_t protocol) {
	// this is a bad idea isn't it
	return socket(domain, type, protocol);
}

struct emu_addrinfo {
	std::int32_t ai_flags;
	std::int32_t ai_family;
	std::int32_t ai_socktype;
	std::int32_t ai_protocol;
	std::uint32_t ai_addrlen;
	std::uint32_t ai_addr_ptr;
	std::uint32_t ai_canonname_ptr;
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
			env.memory_manager().copy(addr_ptr, obj->ai_addr, obj->ai_addrlen);
		}

		auto canonname_ptr = 0u;
		if (obj->ai_canonname != nullptr) {
			auto len = strlen(obj->ai_canonname) + 1;
			canonname_ptr = env.libc().allocate_memory(len);
			env.memory_manager().copy(canonname_ptr, obj->ai_canonname, len);
		}

		emu_addrinfo emu_obj = {
			.ai_flags = obj->ai_flags,
			.ai_family = obj->ai_family,
			.ai_socktype = obj->ai_socktype,
			.ai_protocol = obj->ai_protocol,
			.ai_addrlen = obj->ai_addrlen,
			.ai_addr_ptr = addr_ptr,
			.ai_canonname_ptr = canonname_ptr,
			.ai_next_ptr = copy_impl(env, obj->ai_next, copy_impl)
		};

		env.memory_manager().copy(addr_mem, &emu_obj, sizeof(emu_addrinfo));

		return addr_mem;
	};

	auto r = copy_addr_info(env, result, copy_addr_info);

	freeaddrinfo(result);

	return r;
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

#endif