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
#define REGISTER_STUB_RN(ENV, NAME) \
	ENV.syscall_handler().create_stub_fn( \
		&SyscallTranslator::translate_wrap<&NAME> \
	)

#define REGISTER_FN(ENV, NAME) \
	ENV.program_loader().add_stub_symbol( \
		REGISTER_STUB(ENV, NAME), \
		STR(NAME) \
	)
#define REGISTER_FN_RN(ENV, NAME, SYMBOL) \
	ENV.program_loader().add_stub_symbol( \
		REGISTER_STUB_RN(ENV, NAME), \
		SYMBOL \
	)

#define REGISTER_SYSCALL(ENV, NAME, CALL) \
	ENV.syscall_handler().register_kernel_fn( \
		CALL, \
		&SyscallTranslator::translate_wrap<&kernel_##NAME> \
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
			auto sp_offs = sp + (idx - 4) * 4;

			return env.memory_manager()->read_word(sp_offs);
		}
	}

	inline void push_arg(Environment& env, std::uint32_t idx, std::uint32_t value) {
		if (idx < 4) {
			env.current_cpu()->Regs()[idx] = value;
		} else {
			auto& sp = env.current_cpu()->Regs()[13];
			env.memory_manager()->write_word(sp, value);

			sp -= 4;
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

	template <typename T> requires std::same_as<T, bool>
	inline T translate_reg(Environment& env, std::uint32_t& idx) {
		auto val = pull_arg(env, idx);
		idx++;
		return static_cast<bool>(val);
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

	template <typename T> requires std::same_as<T, bool>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, T value) {
		push_arg(env, idx, static_cast<std::uint32_t>(value));
		idx++;
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

			std::tuple<Environment&, Args...> args{env, SyscallTranslator::translate_reg<Args>(env, idx)...};

			if constexpr (std::is_void_v<R>) {
				std::apply(fn, args);
			} else {
				auto r = std::apply(fn, args);

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

class Variadic {
protected:
	std::uint32_t _arg_idx;
	Environment& _env;

public:
	Variadic(std::uint32_t idx, Environment& env) : _arg_idx(idx), _env(env) {}

	template <typename T>
	T next() {
		return SyscallTranslator::translate_reg<T>(_env, _arg_idx);
	}
};

/**
 * A va_list is like a variadic function,
 * but all args go onto the stack and
 * the given arg is to where that stack offset begins
 */
class VaList : Variadic {
private:
	std::uint32_t _stack_ptr;

public:
	VaList(Environment& env, std::uint32_t stack_begin)
		: Variadic(4, env), _stack_ptr(stack_begin) {}

	template <typename T>
	T next() {
		// this is a hacky way of going about it, but requires the least work
		auto& sp_ptr = _env.current_cpu()->Regs()[13];
		auto sp = sp_ptr;

		sp_ptr = _stack_ptr;
		auto r = Variadic::next<T>();
		sp_ptr = sp;

		return r;
	}
};

namespace SyscallTranslator {
	template <>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, Variadic value) {
		throw std::runtime_error("Translating variadic args is currently unsupported");
	}

	template <>
	inline Variadic translate_reg(Environment& env, std::uint32_t& idx) {
		return Variadic(idx, env);
	}

	template <>
	inline void translate_call_arg(Environment& env, std::uint32_t& idx, VaList value) {
		throw std::runtime_error("Translating variadic args is currently unsupported");
	}

	template <>
	inline VaList translate_reg(Environment& env, std::uint32_t& idx) {
		// this is how they seemed to work from my poking at things
		// sp = base address for args
		auto sp = pull_arg(env, idx);
		idx++;

		return VaList(env, sp);
	}
}

#endif
