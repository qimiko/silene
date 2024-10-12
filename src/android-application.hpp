#pragma once

#ifndef _ANDROID_APPLICATION_HPP
#define _ANDROID_APPLICATION_HPP

#include <vector>
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
	// default initialize with an instance of memory
	ApplicationConfig _config;
	ApplicationState _state{};
	AndroidEnvironment _env;
	Dynarmic::ExclusiveMonitor _monitor{1};

	// creates the first cpu
	void init_cpu();

	// initializes the stack and initial callback addresses
	// should be called before anything involving the memory is performed
	void init_memory();

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
