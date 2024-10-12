#include <cstdint>
#include <cstdio>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "android-application.hpp"
#include "keybind-manager.hpp"
#include "elf.h"
#include "zip-file.h"
#include "glfw-window.h"

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

	std::filesystem::path apk_path{app_apk};
	GlfwAppWindow window{application, {
		.show_cursor_pos = show_cursor_pos,
		.keybind_file = keybind_file,
		.title_name = apk_path.filename().string()
	}};

	if (!window.init()) {
		spdlog::critical("failed to init window");
		return 1;
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

	window.main_loop();

	return 0;
}

