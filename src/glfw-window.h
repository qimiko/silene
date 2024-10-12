#pragma once

#ifndef _GLFW_APP_WINDOW_H
#define _GLFW_APP_WINDOW_H

#ifdef SILENE_USE_EGL
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

#include "base-window.hpp"
#include "keybind-manager.hpp"

class AndroidApplication;

class GlfwAppWindow : public BaseWindow {
	struct WindowConfig {
		bool show_cursor_pos{false};
		std::string keybind_file{};
		std::string title_name{};
	};

	WindowConfig _config;

	float _scale_x{1.0f};
	float _scale_y{1.0f};

	std::unique_ptr<KeybindManager> _keybind_manager{nullptr};

	GLFWwindow* _window{nullptr};

	static void glfw_error_callback(int, const char*);
	static void glfw_mouse_callback(GLFWwindow*, int, int, int);
	static void glfw_mouse_move_callback(GLFWwindow*, double, double);
	static void glfw_key_callback(GLFWwindow*, int, int, int, int);

public:
	virtual std::pair<float, float> framebuffer_size() override;

	virtual bool init() override;
	virtual void main_loop() override;

	GlfwAppWindow(AndroidApplication& app, WindowConfig config);
};

#endif
