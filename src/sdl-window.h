#pragma once

#ifndef _SDL_APP_WINDOW_H
#define _SDL_APP_WINDOW_H

#ifdef SILENE_USE_EGL
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "base-window.hpp"
#include "keybind-manager.hpp"
#include "android-application.hpp"

class SdlAppWindow : public BaseWindow {
	struct WindowConfig {
		bool show_cursor_pos{false};
		std::string keybind_file{};
		std::string title_name{};
	};

	WindowConfig _config;

	SDL_Window* _window{nullptr};
	bool _is_first_frame{false};

	std::unique_ptr<KeybindManager> _keybind_manager{nullptr};

	// store it here as it gets deleted from main
	std::unique_ptr<AndroidApplication> _application;

	float _scale_x{0.0f};
	float _scale_y{0.0f};

	float _fps{};
	std::uint32_t _frame_counter{};
	std::uint64_t _last_tick{};

	void handle_key(SDL_KeyboardEvent& event);

public:
	AndroidApplication& application() const {
		return BaseWindow::application();
	}

	void handle_event(SDL_Event* event);
	void on_quit();

	virtual bool init() override;
	virtual void main_loop() override;

	SdlAppWindow(std::unique_ptr<AndroidApplication>&& app, WindowConfig config);
};

#endif

