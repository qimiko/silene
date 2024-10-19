#pragma once

#ifndef _ANDROID_ENVIRONMENT_HPP
#define _ANDROID_ENVIRONMENT_HPP

#include <cstdint>

#include <dynarmic/interface/A32/a32.h>
#include <dynarmic/interface/A32/config.h>
#include <dynarmic/interface/exclusive_monitor.h>

#include <spdlog/spdlog.h>

#include "android-coprocessor.hpp"
#include "application-state.h"
#include "paged-memory.hpp"
#include "syscall-handler.hpp"
#include "syscall-translator.hpp"
#include "elf-loader.h"
#include "libc-state.h"
#include "jni.h"
#include "gdb-server.h"

#include "environment.h"

class AndroidApplication;

// manages per thread cpu environment
class AndroidEnvironment final : public Dynarmic::A32::UserCallbacks, public Environment {
private:
	bool validate_pointer_addr(std::uint32_t vaddr, bool for_write = false);

	static constexpr auto HALT_REASON_FN_END = Dynarmic::HaltReason::UserDefined1;
	static constexpr auto HALT_REASON_HANDLE_SYSCALL = Dynarmic::HaltReason::UserDefined2;
	static constexpr auto HALT_REASON_ERROR = Dynarmic::HaltReason::UserDefined3;
	static constexpr auto HALT_REASON_KERNEL_SYSCALL = Dynarmic::HaltReason::UserDefined4;

	std::unique_ptr<GdbServer> _debug_server{nullptr};

	std::shared_ptr<Dynarmic::A32::Jit> _cpu{nullptr};
	std::shared_ptr<AndroidCP15> _cp15{std::make_shared<AndroidCP15>()};

	std::int32_t _thread_id{0};

	std::uint64_t ticks_left = 0;

	bool _active = false;
	AndroidApplication& _application;

public:
	std::uint8_t MemoryRead8(std::uint32_t vaddr) override;
	std::uint16_t MemoryRead16(std::uint32_t vaddr) override;
	std::uint32_t MemoryRead32(std::uint32_t vaddr) override;
	std::uint64_t MemoryRead64(std::uint32_t vaddr) override;

	void MemoryWrite8(std::uint32_t vaddr, std::uint8_t value) override;
	void MemoryWrite16(std::uint32_t vaddr, std::uint16_t value) override;
	void MemoryWrite32(std::uint32_t vaddr, std::uint32_t value) override;
	void MemoryWrite64(std::uint32_t vaddr, std::uint64_t value) override;

	bool MemoryWriteExclusive8(std::uint32_t vaddr, std::uint8_t value, std::uint8_t expected) override;
	bool MemoryWriteExclusive16(std::uint32_t vaddr, std::uint16_t value, std::uint16_t expected) override;
	bool MemoryWriteExclusive32(std::uint32_t vaddr, std::uint32_t value, std::uint32_t expected) override;
	bool MemoryWriteExclusive64(std::uint32_t vaddr, std::uint64_t value, std::uint64_t expected) override;

	void InterpreterFallback(std::uint32_t pc, size_t num_instructions) override {
		spdlog::error("interpreter fallback??");
		std::terminate();
	}

	void CallSVC(std::uint32_t swi) override;

	void ExceptionRaised(std::uint32_t pc, Dynarmic::A32::Exception exception) override;

	void AddTicks(std::uint64_t ticks) override {
		if (ticks > ticks_left) {
			ticks_left = 0;
			return;
		}
		ticks_left -= ticks;
	}

	std::uint64_t GetTicksRemaining() override {
		return ticks_left;
	}

	// todo: move all of this into some global orchestrator class

	/**
	 * run a function at some address, assuming that all arguments are already setup
	 */
	void run_func(std::uint32_t vaddr) override;

	template <typename R = void, typename... Args>
	R call_symbol(const std::string_view& symbol, Args... args) {
		// todo: potentially template magic to make this easier
		auto symbol_addr = this->program_loader().get_symbol_addr(symbol);
		if (symbol_addr == 0) {
			throw std::runtime_error("symbol not found");
		}

		return SyscallTranslator::call_func<R>(*this, symbol_addr, args...);
	}

	void begin_debugging();

	virtual std::int32_t thread_id() override {
		return this->_thread_id;
	}

	void dump_state() override;

	std::shared_ptr<Dynarmic::A32::Jit> current_cpu() override {
		return this->_cpu;
	}

	bool is_active() const {
		return this->_active;
	}

	virtual AndroidApplication& application() const override {
		return this->_application;
	}

	AndroidEnvironment(AndroidApplication& application, ApplicationState& state, Dynarmic::ExclusiveMonitor* monitor, std::uint32_t thread_id);
};

#endif
