#pragma once

#ifndef _SYSCALL_HANDLER_HPP
#define _SYSCALL_HANDLER_HPP

#include <bit>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "environment.h"
#include "paged-memory.hpp"

class SyscallHandler {
	std::shared_ptr<PagedMemory> _memory{nullptr};

	using HandlerFunction = void(*)(Environment& env);

	std::unordered_map<std::uint32_t, HandlerFunction> fns{};

public:
	void on_symbol_call(Environment& env);

	/**
	 * creates a stub syscall function and registers it
	 * should return the address of the written function
	*/
	std::uint32_t create_stub_fn(HandlerFunction fn);

	SyscallHandler(std::shared_ptr<PagedMemory> memory) : _memory(memory) {}
};

#endif