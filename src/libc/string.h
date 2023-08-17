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

#endif