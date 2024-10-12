#include "glfw-window.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "android-application.hpp"

void GlfwAppWindow::glfw_error_callback(int error, const char* description) {
	spdlog::error("GLFW Error: {}", description);
}

void GlfwAppWindow::glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return;
	}

	auto env = reinterpret_cast<GlfwAppWindow*>(glfwGetWindowUserPointer(window));

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	env->_application->send_touch(action == GLFW_PRESS, {
		1,
		static_cast<float>(xpos * env->_scale_x),
		static_cast<float>(ypos * env->_scale_y)
	});
}

void GlfwAppWindow::glfw_mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
	auto env = reinterpret_cast<GlfwAppWindow*>(glfwGetWindowUserPointer(window));
	env->_application->move_touches({{
		1,
		static_cast<float>(xpos * env->_scale_x),
		static_cast<float>(ypos * env->_scale_y)
	}});
}

void GlfwAppWindow::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto env = reinterpret_cast<GlfwAppWindow*>(glfwGetWindowUserPointer(window));
	if (auto& keybind_manager = env->_keybind_manager; keybind_manager != nullptr) {
		if (auto touch = keybind_manager->handle(key, scancode, action)) {
			auto [x, y] = touch.value();

			env->_application->send_touch(action == GLFW_PRESS, {
				1,
				static_cast<float>(x),
				static_cast<float>(y)
			});

			return;
		}
	}

	if (action != GLFW_PRESS) {
		return;
	}

	auto key_name = glfwGetKeyName(key, scancode);

	switch (key) {
		case GLFW_KEY_BACKSPACE:
			env->_application->send_ime_delete();
			break;
		case GLFW_KEY_ESCAPE:
			env->_application->send_keydown(4 /* AKEYCODE_BACK */);
			break;
		case GLFW_KEY_SPACE:
			env->_application->send_ime_insert(" ");
			break;
		default:
			env->_application->send_ime_insert(key_name);
			break;
	}
}

bool GlfwAppWindow::init(AndroidApplication& application) {
	this->_application = &application;

	glfwSetErrorCallback(glfw_error_callback);

#if defined(__APPLE__) && defined(SILENE_USE_ANGLE)
	// angle on metal runs significantly better than on desktop gl
	glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_METAL);
#endif

	if (!glfwInit()) {
		return false;
	}

#ifdef SILENE_USE_EGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);

	auto window_title = fmt::format("{} (Silene)", _config.title_name);

	auto window = glfwCreateWindow(1280, 720, window_title.c_str(), nullptr, nullptr);
	this->_window = window;

	if (!window) {
		glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
		window = glfwCreateWindow(1280, 720, window_title.c_str(), nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			return false;
		}
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

#ifndef SILENE_USE_EGL
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		spdlog::error("Failed to initialize GLAD");
		return false;
	}
#endif

	auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

	spdlog::info("gl information: {} ({}) - {}", renderer, vendor, version);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsLight();

	ImGui_ImplGlfw_InitForOpenGL(window, true);

	#ifdef SILENE_USE_EGL
	ImGui_ImplOpenGL3_Init("#version 300 es");
	#else
	ImGui_ImplOpenGL3_Init("#version 120");
	#endif

	glfwSetWindowUserPointer(window, this);

	if (!_config.keybind_file.empty()) {
		_keybind_manager = std::make_unique<KeybindManager>(_config.keybind_file);
	}

	glfwGetWindowContentScale(window, &_scale_x, &_scale_y);

	return true;
}

std::pair<float, float> GlfwAppWindow::framebuffer_size() {
	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);

	return {width, height};
}

void GlfwAppWindow::main_loop() {
	// this prevents issues if callbacks happen pre-init
	glfwSetMouseButtonCallback(_window, &glfw_mouse_callback);
	glfwSetCursorPosCallback(_window, &glfw_mouse_move_callback);
	glfwSetKeyCallback(_window, &glfw_key_callback);

	auto last_time = glfwGetTime();
	auto last_update_time = glfwGetTime();
	auto accumulated_time = 0.0f;

	while (!glfwWindowShouldClose(_window)) {
		auto current_time = glfwGetTime();
    auto dt = current_time - last_time;

		last_time = current_time;
    accumulated_time += dt;

		if (accumulated_time < (1.0 / 60.0)) {
			continue;
		}

		auto update_dt = current_time - last_update_time;
    last_update_time = current_time;
    accumulated_time = 0.0f;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("Info Dialog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("FPS: %.0f", 1.0/update_dt);

			if (_config.show_cursor_pos) {
				double xpos, ypos;

				glfwGetCursorPos(_window, &xpos, &ypos);

				ImGui::Text("X: %.0f | Y: %.0f", xpos * _scale_x, ypos * _scale_y);
			}
		}

		ImGui::End();

		ImGui::Render();

		_application->draw_frame();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(_window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(_window);
	glfwTerminate();

	_window = nullptr;
}

GlfwAppWindow::GlfwAppWindow(WindowConfig config) : _config{config} {}
