#pragma once

#ifndef _ANDROID_ENVIRONMENT_HPP
#define _ANDROID_ENVIRONMENT_HPP

#include <cstdint>

#include <dynarmic/interface/A32/a32.h>
#include <dynarmic/interface/A32/config.h>

#include <spdlog/spdlog.h>

#include "paged-memory.hpp"
#include "syscall-handler.hpp"
#include "syscall-translator.hpp"
#include "elf-loader.h"
#include "libc-state.h"
#include "jni.h"

#include "environment.h"

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

class AndroidEnvironment final : public Dynarmic::A32::UserCallbacks, public Environment {
public:
	static constexpr auto HALT_REASON_FN_END = Dynarmic::HaltReason::UserDefined1;
	static constexpr auto HALT_REASON_HANDLE_SYSCALL = Dynarmic::HaltReason::UserDefined2;
	static constexpr auto HALT_REASON_ERROR = Dynarmic::HaltReason::UserDefined3;

	// default initialize with an instance of memory
	std::shared_ptr<PagedMemory> _memory{
		std::make_shared<PagedMemory>()
	};

	Elf::Loader _program_loader{_memory};
	SyscallHandler _syscall_handler{_memory};
	LibcState _libc{_memory};
	JniState _jni{_memory};

	std::shared_ptr<Dynarmic::A32::Jit> _cpu{nullptr};

	std::uint64_t ticks_left = 0;

	std::uint8_t MemoryRead8(std::uint32_t vaddr) override {
		return _memory->read_byte(vaddr);
	}

	u16 MemoryRead16(u32 vaddr) override {
		return _memory->read_halfword(vaddr);
	}

	u32 MemoryRead32(u32 vaddr) override {
		return _memory->read_word(vaddr);
	}

	u64 MemoryRead64(u32 vaddr) override {
		return _memory->read_doubleword(vaddr);
	}

	void MemoryWrite8(u32 vaddr, u8 value) override {
		_memory->write_byte(vaddr, value);
	}

	void MemoryWrite16(u32 vaddr, u16 value) override {
		_memory->write_halfword(vaddr, value);
	}

	void MemoryWrite32(u32 vaddr, u32 value) override {
		_memory->write_word(vaddr, value);
	}

	void MemoryWrite64(u32 vaddr, u64 value) override {
		_memory->write_doubleword(vaddr, value);
	}

	// todo: obviously this won't work once threads are introduced
	// figure things out when that happens
	bool MemoryWriteExclusive8(std::uint32_t vaddr, std::uint8_t value, std::uint8_t expected) override {
		_memory->write_byte(vaddr, value);
		return true;
	}

  bool MemoryWriteExclusive16(std::uint32_t vaddr, std::uint16_t value, std::uint16_t expected) override {
		_memory->write_halfword(vaddr, value);
		return true;
	}

  bool MemoryWriteExclusive32(std::uint32_t vaddr, std::uint32_t value, std::uint32_t expected) override {
		_memory->write_word(vaddr, value);
		return true;
	}

	bool MemoryWriteExclusive64(std::uint32_t vaddr, std::uint64_t value, std::uint64_t expected) override {
		_memory->write_doubleword(vaddr, value);
		return true;
	}

	void InterpreterFallback(u32 pc, size_t num_instructions) override {
		spdlog::error("interpreter fallback??");
		std::terminate();
	}

	void CallSVC(std::uint32_t swi) override;

	void ExceptionRaised(std::uint32_t pc, Dynarmic::A32::Exception exception) override;

	void AddTicks(u64 ticks) override {
		if (ticks > ticks_left) {
			ticks_left = 0;
			return;
		}
		ticks_left -= ticks;
	}

	u64 GetTicksRemaining() override {
		return ticks_left;
	}

	std::shared_ptr<PagedMemory> memory_manager() override {
		return this->_memory;
	}

	Elf::Loader& program_loader() override {
		return this->_program_loader;
	}

	// todo: move all of this into some global orchestrator class

	/**
	 * run a function at some address, assuming that all arguments are already setup
	 */
	void run_func(std::uint32_t vaddr) override;


	template <typename R = void, typename... Args>
	R call_symbol(const std::string_view& symbol, Args... args) {
		// todo: potentially template magic to make this easier
		auto symbol_addr = this->_program_loader.get_symbol_addr(symbol);
		if (symbol_addr == 0) {
			throw std::runtime_error("symbol not found");
		}

		return SyscallTranslator::call_func<R>(*this, symbol_addr, args...);
	}

	bool has_symbol(const std::string_view& symbol) const {
		return this->_program_loader.has_symbol(symbol);
	}

	// should be called before anything involving the memory is performed
	void pre_init();

	// should be called after all program loading is done
	void post_init();

	void set_cpu(std::shared_ptr<Dynarmic::A32::Jit> cpu) {
		this->_cpu = cpu;
	}

	std::shared_ptr<Dynarmic::A32::Jit> current_cpu() override {
		return this->_cpu;
	}

	LibcState& libc() override {
		return this->_libc;
	}

	JniState& jni() override {
		return this->_jni;
	}

	SyscallHandler& syscall_handler() override {
		return this->_syscall_handler;
	}

	AndroidEnvironment() = default;
};

#endif
