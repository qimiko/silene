#include <cstdint>
#include <cstdio>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "android-application.hpp"
#include "keybind-manager.hpp"
#include "elf.h"
#include "zip-file.h"

#ifdef SILENE_USE_EGL
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

void glfw_error_callback(int error, const char* description) {
	spdlog::error("GLFW Error: {}", description);
}

// TODO: move this into a separate class
float glfw_scale_x = 1.0f;
float glfw_scale_y = 1.0f;

KeybindManager* keybind_manager = nullptr;

void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return;
	}

	if (auto env = reinterpret_cast<AndroidApplication*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		env->send_touch(action == GLFW_PRESS, {
			1,
			static_cast<float>(xpos * glfw_scale_x),
			static_cast<float>(ypos * glfw_scale_y)
		});
	}
}

void glfw_mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
	if (auto env = reinterpret_cast<AndroidApplication*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		env->move_touches({{
			1,
			static_cast<float>(xpos * glfw_scale_x),
			static_cast<float>(ypos * glfw_scale_y)
		}});
	}
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (auto env = reinterpret_cast<AndroidApplication*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		if (keybind_manager != nullptr) {
			if (auto touch = keybind_manager->handle(key, scancode, action)) {
				auto [x, y] = touch.value();

				env->send_touch(action == GLFW_PRESS, {
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
				env->send_ime_delete();
				break;
			case GLFW_KEY_ESCAPE:
				env->send_keydown(4 /* AKEYCODE_BACK */);
				break;
			case GLFW_KEY_SPACE:
				env->send_ime_insert(" ");
				break;
			default:
				env->send_ime_insert(key_name);
				break;
		}
	}
}

int main(int argc, char** argv) {
	CLI::App app;
	argv = app.ensure_utf8(argv);

	std::string app_apk;
	app.add_option("apk", app_apk, "path to apk file to use for libraries")
		->check(CLI::ExistingFile)
		->required();

	bool verbose = false;
	app.add_flag("-v,--verbose", verbose, "enable extra debug logging");

	bool enable_debugging = false;
	app.add_flag("-d,--debug", enable_debugging, "enables debugging through gdb on port 5039");

	std::string app_resources{};
	app.add_option("--resources", app_resources, "Determines the APK file to use for resources. If left blank, the main APK file is used")
		->check(CLI::ExistingFile);

	std::string support_dir = "./support/";
	app.add_option("--link", support_dir, "path to load support binaries from")
		->capture_default_str()
		->check(CLI::ExistingDirectory);

	bool show_cursor_pos = false;
	app.add_flag("--show-cursor-pos", show_cursor_pos, "enables showing the cursor position on the info dialog. useful for making keybinds");

	std::string keybind_file;
	app.add_option("--keybinds", keybind_file, "path to keybind file, if unspecified keybinding is disabled")
		->check(CLI::ExistingFile);

	CLI11_PARSE(app, argc, argv);

	if (app_resources.empty()) {
		app_resources = app_apk;
	}

	if (verbose) {
		spdlog::set_level(spdlog::level::debug);
	}

	AndroidApplication application{{enable_debugging, app_resources}};

	ZipFile apk_file{app_apk};

	std::string lib_path;
	if (apk_file.has_file("lib/armeabi-v7a/libgame.so")) {
		lib_path = "lib/armeabi-v7a/libgame.so";
	} else if (apk_file.has_file("lib/armeabi-v7a/libcocos2dcpp.so")) {
		lib_path = "lib/armeabi-v7a/libcocos2dcpp.so";
	} else if (apk_file.has_file("lib/armeabi/libgame.so")) {
		lib_path = "lib/armeabi/libgame.so"; // pre 1.6 only has armv5
	} else {
		spdlog::error("apk is missing library for a supported architecture");
		return 1;
	}

	auto main_lib = apk_file.read_file_bytes(lib_path);

	auto elf = Elf::File(std::move(main_lib));

	std::filesystem::path support_path{support_dir};

	/*
	auto libc_path = support_path / "libc.so";
	auto libc = Elf::File(libc_path.string());
	*/

	auto zlib_path = support_path / "libz.so";
	auto zlib = Elf::File(zlib_path.string());

	glfwSetErrorCallback(glfw_error_callback);

#if defined(__APPLE__) && defined(SILENE_USE_ANGLE)
	// angle on metal runs significantly better than on desktop gl
	glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE, GLFW_ANGLE_PLATFORM_TYPE_METAL);
#endif

	if (!glfwInit())
		return 1;

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

	std::filesystem::path apk_path{app_apk};
	auto window_title = fmt::format("{} (Silene)", apk_path.filename().string());

	auto window = glfwCreateWindow(1280, 720, window_title.c_str(), nullptr, nullptr);
	if (!window) {
		glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
		window = glfwCreateWindow(1280, 720, window_title.c_str(), nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			return 1;
		}
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

#ifndef SILENE_USE_EGL
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		spdlog::error("Failed to initialize GLAD");
		return 1;
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

	glfwSetWindowUserPointer(window, &application);

	if (!keybind_file.empty()) {
		keybind_manager = new KeybindManager(keybind_file);
	}

	application.init();

	/*
	env.set_assets_dir(resources_dir);
	*/

	/*
	env.program_loader().map_elf(libc);
	env.post_load();
	*/

	application.load_library(zlib);
	application.load_library(elf);

	application.finalize_libraries();

	spdlog::info("init fns done");

	// order of fns:
	// JNI_OnLoad
	// Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath
	// Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit

	spdlog::info("beginning JNI init");

	application.init_jni();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	application.init_game(width, height);

	auto last_time = glfwGetTime();
	auto last_update_time = glfwGetTime();
	auto accumulated_time = 0.0f;

	glfwGetWindowContentScale(window, &glfw_scale_x, &glfw_scale_y);

	glfwSetMouseButtonCallback(window, &glfw_mouse_callback);
	glfwSetCursorPosCallback(window, &glfw_mouse_move_callback);
	glfwSetKeyCallback(window, &glfw_key_callback);

	while (!glfwWindowShouldClose(window)) {
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

			if (show_cursor_pos) {
				double xpos, ypos;

				glfwGetCursorPos(window, &xpos, &ypos);

				ImGui::Text("X: %.0f | Y: %.0f", xpos * glfw_scale_x, ypos * glfw_scale_y);
			}
		}

		ImGui::End();

		ImGui::Render();

		application.draw_frame();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

