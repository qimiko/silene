#pragma once

#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H

#include <cstring>

#include "../environment.h"

std::int32_t emu_strcmp(Environment& env, std::uint32_t lhs_ptr, std::uint32_t rhs_ptr) {
	auto lhs = env.memory_manager()->read_bytes<char>(lhs_ptr);
	auto rhs = env.memory_manager()->read_bytes<char>(rhs_ptr);

	return std::strcmp(lhs, rhs);
}

std::int32_t emu_strncmp(Environment& env, std::uint32_t lhs_ptr, std::uint32_t rhs_ptr, std::uint32_t count) {
	auto lhs = env.memory_manager()->read_bytes<char>(lhs_ptr);
	auto rhs = env.memory_manager()->read_bytes<char>(rhs_ptr);

	return std::strncmp(lhs, rhs, count);
}

std::uint32_t emu_strncpy(Environment& env, std::uint32_t destination_ptr, std::uint32_t source_ptr, std::uint32_t num) {
	auto destination = env.memory_manager()->read_bytes<char>(destination_ptr);
	auto source = env.memory_manager()->read_bytes<char>(source_ptr);

	std::strncpy(destination, source, num);

	return destination_ptr;
}

std::uint32_t emu_strcpy(Environment& env, std::uint32_t destination_ptr, std::uint32_t source_ptr) {
	auto destination = env.memory_manager()->read_bytes<char>(destination_ptr);
	auto source = env.memory_manager()->read_bytes<char>(source_ptr);

	std::strcpy(destination, source);

	return destination_ptr;
}

std::uint32_t emu_memcpy(Environment& env, std::uint32_t destination_ptr, std::uint32_t source_ptr, std::uint32_t num) {
	auto destination = env.memory_manager()->read_bytes<std::uint8_t>(destination_ptr);
	auto source = env.memory_manager()->read_bytes<std::uint8_t>(source_ptr);

	std::memcpy(destination, source, num);

	// memcpy returns the destination, which would be wrong
	return destination_ptr;
}

std::uint32_t emu_memmove(Environment& env, std::uint32_t destination_ptr, std::uint32_t source_ptr, std::uint32_t num) {
	auto destination = env.memory_manager()->read_bytes<std::uint8_t>(destination_ptr);
	auto source = env.memory_manager()->read_bytes<std::uint8_t>(source_ptr);

	std::memmove(destination, source, num);

	return destination_ptr;
}

std::uint32_t emu_strlen(Environment& env, std::uint32_t str_ptr) {
	auto str = env.memory_manager()->read_bytes<char>(str_ptr);
	return std::strlen(str);
}

std::uint32_t emu_memset(Environment& env, std::uint32_t src_ptr, std::int32_t value, std::uint32_t num) {
	auto source = env.memory_manager()->read_bytes<char>(src_ptr);
	std::memset(source, value, num);

	return src_ptr;
}

std::int32_t emu_memcmp(Environment& env, std::uint32_t lhs_ptr, std::uint32_t rhs_ptr, std::uint32_t num) {
	auto lhs = env.memory_manager()->read_bytes<char>(lhs_ptr);
	auto rhs = env.memory_manager()->read_bytes<char>(rhs_ptr);

	return std::memcmp(lhs, rhs, num);
}

std::uint32_t emu_memchr(Environment& env, std::uint32_t ptr, std::int32_t ch, std::uint32_t count) {
	auto source = env.memory_manager()->read_bytes<std::uint8_t>(ptr);

	auto r = reinterpret_cast<std::uint8_t*>(std::memchr(source, ch, count));

	if (r == 0) {
		return 0;
	}

	auto offset = reinterpret_cast<std::ptrdiff_t>(r - source);

	return ptr + static_cast<std::uint32_t>(offset);
}

std::uint32_t emu_strstr(Environment& env, std::uint32_t str_ptr, std::uint32_t substr_ptr) {
	auto str = env.memory_manager()->read_bytes<char>(str_ptr);
	auto substr = env.memory_manager()->read_bytes<char>(substr_ptr);

	auto r = reinterpret_cast<char*>(std::strstr(str, substr));

	if (r == 0) {
		return 0;
	}

	auto offset = reinterpret_cast<std::ptrdiff_t>(r - str);

	return str_ptr + static_cast<std::uint32_t>(offset);
}

std::uint32_t emu_strtok(Environment& env, std::uint32_t str_ptr, std::uint32_t delim_ptr) {
	auto delim = env.memory_manager()->read_bytes<char>(delim_ptr);

	if (str_ptr == 0) {
		str_ptr = env.libc().get_strtok_buffer();
	}

	auto str = env.memory_manager()->read_bytes<char>(str_ptr);

	auto substr_offs = std::strspn(str, delim);

	str_ptr += substr_offs;
	str += substr_offs;

	if (*str == '\0') {
		env.libc().set_strtok_buffer(str_ptr);
		return 0;
	}

	auto begin_token = str_ptr;

	auto token_len = std::strcspn(str, delim);
	str_ptr += token_len;
	str += token_len;

	if (*str != '\0') {
		*str = '\0';
		str_ptr++;
	}

	env.libc().set_strtok_buffer(str_ptr);
	return begin_token;
}

#endif