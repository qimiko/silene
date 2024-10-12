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

class Environment;
class PagedMemory;

class SyscallHandler {
	PagedMemory& _memory;

	using HandlerFunction = void(*)(Environment& env);

	std::unordered_map<std::uint32_t, HandlerFunction> fns{};
	std::unordered_map<std::uint32_t, HandlerFunction> _kernel_fns{};

public:
	void on_symbol_call(Environment& env);
	void on_kernel_call(Environment& env);

	/**
	 * creates a stub syscall function and registers it
	 * should return the address of the written function
	*/
	std::uint32_t create_stub_fn(HandlerFunction fn);

	/**
	 * registers a syscall for use in libc
	 */
	void register_kernel_fn(std::uint32_t call, HandlerFunction fn);

	/**
	 * writes a stub to an existing place in memory.
	 * at least 4/8 bytes (thumb/arm) is required to override the function
	 */
	std::uint32_t replace_fn(std::uint32_t addr, HandlerFunction fn);

	SyscallHandler(PagedMemory& memory) : _memory(memory) {}
};

#endif