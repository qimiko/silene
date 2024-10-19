#pragma once

#ifndef _ANDROID_APPLICATION_HPP
#define _ANDROID_APPLICATION_HPP

#include <array>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>

#include <dynarmic/interface/exclusive_monitor.h>

#include "application-state.h"
#include "android-environment.hpp"
#include "elf.h"

// manages global application state
class AndroidApplication : public StateHolder {
public:
	struct ApplicationConfig {
		bool debug{false};
		std::string resources{};
	};

private:
	static constexpr std::uint32_t MAX_PROCESSORS = 4;

	// default initialize with an instance of memory
	ApplicationConfig _config;
	ApplicationState _state{};

	std::uint32_t _last_tid{1};
	std::unordered_map<std::uint32_t, AndroidEnvironment> _envs{};
	std::unordered_map<std::uint32_t, std::thread> _unclaimed_threads{};

	std::mutex _envs_mutex{};

	Dynarmic::ExclusiveMonitor _monitor{MAX_PROCESSORS};

	// primary env for function calls
	AndroidEnvironment _env;

	// initializes the stack and initial callback addresses
	// should be called before anything involving the memory is performed
	void init_memory();

	void create_processor_with_func(std::uint32_t start_addr, std::uint32_t arg, std::uint32_t id);

public:
	// creates the initial application state
	void init();

	// loads a library into memory
	void load_library(Elf::File& lib);

	// calls the init functions for each loaded library
	void finalize_libraries();

	// calls jni_onload, if the symbol exists
	void init_jni();

	// begins game initialization
	void init_game(int width, int height);

	std::uint32_t create_thread(std::uint32_t start_addr, std::uint32_t arg);
	bool detach_thread(std::uint32_t thread_id);
	bool join_thread(std::uint32_t thread_id, std::uint32_t exit_value);

	void draw_frame();

	struct TouchData {
		std::uint32_t id;
		float x;
		float y;
	};

	void send_touch(bool is_push, TouchData touch);
	void move_touches(const std::vector<TouchData>&);
	void send_ime_insert(std::string data);
	void send_ime_delete();
	void send_keydown(int android_keycode);

	AndroidApplication(ApplicationConfig config);

	// these should already be implicit
	AndroidApplication(const AndroidApplication&) = delete;
	AndroidApplication& operator=(AndroidApplication&) = delete;
};

#endif
