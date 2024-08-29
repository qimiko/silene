#pragma once

#ifndef _SYSCALL_TRANSLATOR_HPP
#define _SYSCALL_TRANSLATOR_HPP

#include <concepts>
#include <type_traits>

#include "environment.h"

#define STR(X) #X
#define REGISTER_STUB(ENV, NAME) \
	ENV.syscall_handler().create_stub_fn( \
		&SyscallTranslator::translate_wrap<&emu_##NAME> \
	)
#define REGISTER_FN(ENV, NAME) \
	ENV.program_loader().add_stub_symbol( \
		REGISTER_STUB(ENV, NAME), \
		STR(NAME) \
	)

namespace SyscallTranslator {
	namespace {
		/**
		 * utility type that determines if type T is same as one of the following types
		 */
		template <typename T, typename... Types>
		concept is_one_of = (std::same_as<T, Types> || ...);
	}

	inline std::uint32_t pull_arg(Environment& env, std::uint32_t idx) {
		if (idx < 4) {
			// r0 - r3 stores the first 4 arguments
			return env.current_cpu()->Regs()[idx];
		} else {
			// otherwise, our value is on the stack
			auto sp = env.current_cpu()->Regs()[13];
			auto sp_offs = (idx - 4) * 4;

			return env.memory_manager()->read_word(sp_offs);
		}
	}

	inline void push_arg(Environment& env, std::uint32_t idx, std::uint32_t value) {
		if (idx < 4) {
			env.current_cpu()->Regs()[idx] = value;
		} else {
			auto sp = env.current_cpu()->Regs()[13];
			return env.memory_manager()->write_word(sp, value);

			env.current_cpu()->Regs()[13] -= 4;
		}
	}

	template <typename T> requires is_one_of<T, std::uint32_t, std::int32_t, float>
	inline T translate_reg(Environment& env, std::uint32_t& idx) {
		auto val = pull_arg(env, idx);
		idx++;
		return std::bit_cast<T>(val);
	}

	template <typename T> requires is_one_of<T, std::uint64_t, std::int64_t, double>
	inline T translate_reg(Environment& env, std::uint32_t& idx) {
		auto val = std::bit_cast<T>(
			static_cast<std::uint64_t>(pull_arg(env, idx)) |
			static_cast<std::uint64_t>(pull_arg(env, idx + 1)) << 32
		);
		idx += 2;

		return val;
	}

	/**
	 * defines translating an emulator register to some value
	 */
	template <typename T>
	inline T translate_reg(Environment& env, std::uint32_t& idx) = delete;

	template <typename T> requires is_one_of<T, std::uint32_t, std::int32_t, float>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, T value) {
		push_arg(env, idx, std::bit_cast<std::uint32_t>(value));
		idx++;
	}

	template <typename T> requires is_one_of<T, std::uint64_t, std::int64_t, double>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, T value) {
		auto data = std::bit_cast<std::uint64_t>(value);
		push_arg(env, idx, static_cast<std::uint32_t>(data));
		push_arg(env, idx + 1, static_cast<std::uint32_t>(data >> 32));
		idx += 2;
	}

	/**
	 * defines translating some value to an emulator register
	 */
	template <typename T>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, T value) = delete;

	/**
	 * defines a type that can be translated to/from the emulator context
	 */
	template <typename T>
	concept Translatable = std::is_void_v<T> || requires(T value, Environment& env, std::uint32_t& idx_ref) {
		{ SyscallTranslator::translate_reg<T>(env, idx_ref) } -> std::same_as<T>;
		{ SyscallTranslator::translate_call_arg<T>(env, idx_ref, value) };
	};

	namespace {
		template <Translatable R, Translatable... Args>
		inline void translate_wrap_helper(R(*fn)(Environment& f, Args... args), Environment& env) {
			auto idx = 0u;

			if constexpr (std::is_void_v<R>) {
				fn(env, SyscallTranslator::translate_reg<Args>(env, idx)...);
			} else {
				auto r = fn(env, SyscallTranslator::translate_reg<Args>(env, idx)...);

				auto return_idx = 0u;
				SyscallTranslator::translate_call_arg(env, return_idx, r);
			}
		}
	}

	/**
	 * translates an emulator call to a function call
	 */
	template <const auto F>
	void translate_wrap(Environment& env) {
		SyscallTranslator::translate_wrap_helper(F, env);
	}

	/**
	 * calls a function from the emulator
	 * also sets up args and return values
	 */
	template <Translatable R = void, Translatable... Args>
	R call_func(Environment& env, std::uint32_t vaddr, Args... args) {
		// in this case, the caller has to restore the stack
		auto sp = env.current_cpu()->Regs()[13];
		auto arg_idx = 0u;

		(SyscallTranslator::translate_call_arg(env, arg_idx, args), ...);
		env.run_func(vaddr);

		env.current_cpu()->Regs()[13] = sp;

		if constexpr (!std::is_void_v<R>) {
			// this int is wasteful...
			auto return_idx = 0u;
			return SyscallTranslator::translate_reg<R>(env, return_idx);
		}
	}
}

#endif
