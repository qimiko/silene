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

#define STR(X) #X
#define REGISTER_FN(ENV, NAME) \
	ENV.program_loader().add_stub_symbol( \
		ENV.syscall_handler().create_stub_fn( \
			&SyscallHandler::translate_wrap<&emu_##NAME> \
		), \
		STR(NAME) \
	)

class SyscallHandler {
	std::shared_ptr<PagedMemory> _memory{nullptr};

	using HandlerFunction = void(*)(Environment& env);

	std::unordered_map<std::uint32_t, HandlerFunction> fns{};

	static inline void setup_call_arg(Environment& env, int idx) {
		// default case, no code necessary
	}

	// if data is double width, with the first half of data placed after the second
	template <typename T>
	static inline constexpr bool is_le_double_width_v = std::is_same_v<T, double> || std::is_same_v<T, std::uint64_t>;

	template <typename F, typename... Args>
	static inline void setup_call_arg(Environment& env, int idx, F first, Args... remain) {
		// todo: multiple size args
		if constexpr (!is_le_double_width_v<F>) {
			env.current_cpu()->Regs()[idx] = std::bit_cast<std::uint32_t>(first);
			setup_call_arg(env, idx + 1, remain...);
		} else {
			auto data = std::bit_cast<std::uint64_t>(first);
			env.current_cpu()->Regs()[idx] = static_cast<std::uint32_t>(data);
			env.current_cpu()->Regs()[idx + 1] = static_cast<std::uint32_t>(data >> 32);
			setup_call_arg(env, idx + 2, remain...);
		}
	}

	template <typename T>
	static inline T translate_reg(Environment& env, std::uint32_t& idx) {

		if constexpr (!is_le_double_width_v<T>) {
			auto val = env.current_cpu()->Regs()[idx];
			idx++;
			return std::bit_cast<T>(val);
		} else {
			auto val = std::bit_cast<T>(
				static_cast<std::uint64_t>(env.current_cpu()->Regs()[idx]) |
				static_cast<std::uint64_t>(env.current_cpu()->Regs()[idx + 1]) << 32
			);
			idx += 2;

			return val;
		}
	}

	template <typename R, typename... Args>
	static inline void translate_wrap_helper(R(*fn)(Environment& f, Args... args), Environment& env) {
		auto idx = 0u;

		if constexpr (std::is_void_v<R>) {
			fn(env, translate_reg<Args>(env, idx)...);
		} else {
			auto r = fn(env, translate_reg<Args>(env, idx)...);
			setup_call_arg(env, 0, r);
		}
	}

public:
	void on_symbol_call(Environment& env);

	/**
	 * creates a stub syscall function and registers it
	 * should return the address of the written function
	*/
	std::uint32_t create_stub_fn(HandlerFunction fn);

	SyscallHandler(std::shared_ptr<PagedMemory> memory) : _memory(memory) {}

	template <const auto F>
	static void translate_wrap(Environment& env) {
		translate_wrap_helper(F, env);
	}

	// todo: handle 64-bit types

	/**
	 * calls a function from the emulator
	 * also sets up args and return values
	*/
	template <typename R = void, typename... Args>
	static R call_func(Environment& env, std::uint32_t vaddr, Args... args) {
		setup_call_arg(env, 0, args...);

		env.run_func(vaddr);

		if constexpr (!std::is_void_v<R>) {
			// this int is wasteful...
			auto _ = 0u;
			return translate_reg<R>(env, _);
		}
	}
};

#endif