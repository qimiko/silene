#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <dynarmic/interface/A32/a32.h>
#include <dynarmic/interface/A32/config.h>
#include <dynarmic/interface/exclusive_monitor.h>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "android-environment.hpp"
#include "android-coprocessor.hpp"
#include "paged-memory.hpp"
#include "elf.h"
#include "elf-loader.h"
#include "zip-file.h"

#include "hook/fileutils.h"

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

void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return;
	}

	if (auto env = reinterpret_cast<AndroidEnvironment*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		auto jni_env_ptr = env->jni().get_env_ptr();
		double xpos, ypos;

		glfwGetCursorPos(window, &xpos, &ypos);

		if (action == GLFW_PRESS) {
			env->call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin",
				jni_env_ptr, 0, 1, static_cast<float>(xpos * glfw_scale_x), static_cast<float>(ypos * glfw_scale_y));
		} else if (action == GLFW_RELEASE) {
			env->call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd",
				jni_env_ptr, 0, 1, static_cast<float>(xpos * glfw_scale_x), static_cast<float>(ypos * glfw_scale_y));
		}
	}
}

void glfw_mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
	if (auto env = reinterpret_cast<AndroidEnvironment*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		auto jni_env_ptr = env->jni().get_env_ptr();

		auto ids = env->jni().create_ref(std::vector{ 1 });
		auto xs = env->jni().create_ref(std::vector{static_cast<float>(xpos * glfw_scale_x)});
		auto ys = env->jni().create_ref(std::vector{static_cast<float>(ypos * glfw_scale_y)});

		env->call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove", jni_env_ptr, 0, ids, xs, ys);
	}
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key != GLFW_KEY_ESCAPE || action != GLFW_PRESS) {
		return;
	}

	if (auto env = reinterpret_cast<AndroidEnvironment*>(glfwGetWindowUserPointer(window)); env != nullptr) {
		auto jni_env_ptr = env->jni().get_env_ptr();
		env->call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyDown", jni_env_ptr, 0, 4 /* AKEYCODE_BACK */);
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

	CLI11_PARSE(app, argc, argv);

	if (app_resources.empty()) {
		app_resources = app_apk;
	}

	if (verbose) {
		spdlog::set_level(spdlog::level::debug);
	}

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

	auto header = elf.header();

	if (
    header->type != Elf::Type::Shared ||
		header->machine != Elf::Machine::Armv7
	) {
		std::cout << "elf file does not match required parameters" << std::endl;
		return 1;
	}

	std::filesystem::path support_path{support_dir};

	/*
	auto libc_path = support_path / "libc.so";
	auto libc = Elf::File(libc_path.string());
	*/

	auto zlib_path = support_path / "libz.so";
	auto zlib = Elf::File(zlib_path.string());

	glfwSetErrorCallback(glfw_error_callback);

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

	auto window = glfwCreateWindow(640, 480, "Silene", nullptr, nullptr);
	if (!window) {
		glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
		window = glfwCreateWindow(640, 480, "Silene", nullptr, nullptr);
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

	AndroidEnvironment env{};
	glfwSetWindowUserPointer(window, &env);

	auto cp15 = std::make_shared<AndroidCP15>();

	Dynarmic::A32::UserConfig user_config{};

	auto monitor = Dynarmic::ExclusiveMonitor{1};
	// this feels like a hack..?
	user_config.processor_id = 0;
	user_config.global_monitor = &monitor;

	user_config.coprocessors[15] = cp15;

	// user_config.very_verbose_debugging_output = true;
	user_config.callbacks = &env;

	auto cpu = std::make_shared<Dynarmic::A32::Jit>(user_config);
	env.set_cpu(cpu);

	/*
	env.set_assets_dir(resources_dir);
	*/

	if (enable_debugging) {
		env.begin_debugging();
	}

	env.pre_init();

	/*
	env.program_loader().map_elf(libc);
	env.post_load();
	*/

	env.program_loader().map_elf(zlib);
	env.post_load();

	env.program_loader().map_elf(elf);
	env.post_load();

	spdlog::info("init fns done");

	// order of fns:
	// JNI_OnLoad
	// Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath
	// Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit

	spdlog::info("beginning JNI init");

	auto jni_env_ptr = env.jni().get_env_ptr();

	if (env.has_symbol("JNI_OnLoad")) {
		auto jvm_ptr = env.jni().get_vm_ptr();
		env.call_symbol<void>("JNI_OnLoad", jvm_ptr);
	}

/*
	if (auto filedata = env.program_loader().get_symbol_addr("_ZN7cocos2d18CCFileUtilsAndroid11getFileDataEPKcS2_Pm"); filedata != 0) {
		env.syscall_handler().replace_fn(filedata, &SyscallTranslator::translate_wrap<&hook_CCFileUtils_getFileData>);
	} else if (auto legacy_filedata = env.program_loader().get_symbol_addr("_ZN7cocos2d11CCFileUtils11getFileDataEPKcS2_Pm"); legacy_filedata != 0) {
		env.syscall_handler().replace_fn(legacy_filedata, &SyscallTranslator::translate_wrap<&hook_CCFileUtils_getFileData>);
	}
*/

	glfwSetMouseButtonCallback(window, &glfw_mouse_callback);
	glfwSetCursorPosCallback(window, &glfw_mouse_move_callback);
	glfwSetKeyCallback(window, &glfw_key_callback);

	env.libc().expose_file("/application_resources.apk", app_resources);

	// first arg should be a jstring to the path
	auto path_string = env.jni().create_string_ref("/application_resources.apk");

	if (env.has_symbol("Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPaths")) {
		// this symbol was used on older versions of cocos (but are otherwise identical)
		env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPaths", jni_env_ptr, 0, path_string);
	} else {
		env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath", jni_env_ptr, 0, path_string);
	}

	env.jni().remove_ref(path_string);

	// last two args are width/height

	auto init_finished = false;

	auto last_time = glfwGetTime();
	auto last_update_time = glfwGetTime();
	auto accumulated_time = 0.0f;

	glfwGetWindowContentScale(window, &glfw_scale_x, &glfw_scale_y);

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

		if (!init_finished) {
			ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
			if (ImGui::Begin("Loading Dialog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Loading...");
			}
		} else {
			ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
			if (ImGui::Begin("Info Dialog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("FPS: %.0f", 1.0/update_dt);
			}
		}

		ImGui::End();

		ImGui::Render();

		if (init_finished) {
			env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender", jni_env_ptr, 0);
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit", jni_env_ptr, 0, width, height);
			spdlog::info("init finished");

			init_finished = true;
		}

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

