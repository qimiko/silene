#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <memory>

#include "sdl-window.h"
#include "android-application.hpp"
#include "keybind-manager.hpp"
#include "elf.h"
#include "zip-file.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
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

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError& e) {
		return SDL_APP_FAILURE;
	}

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
		return SDL_APP_FAILURE;
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
	
	auto application = std::unique_ptr<AndroidApplication>(new AndroidApplication({enable_debugging, app_resources}));
	auto window = new SdlAppWindow(std::move(application), {
		.show_cursor_pos = show_cursor_pos,
		.keybind_file = keybind_file,
		.title_name = apk_path.filename().string()
	});

	if (!window->init()) {
		spdlog::critical("failed to init window");

		delete window;
		return SDL_APP_FAILURE;
	}
	
	*appstate = reinterpret_cast<void*>(window);

	window->application().init();

	/*
	env.set_assets_dir(resources_dir);
	*/

	/*
	env.program_loader().map_elf(libc);
	env.post_load();
	*/

	window->application().load_library(zlib);
	window->application().load_library(elf);

	window->application().finalize_libraries();

	spdlog::info("init fns done, beginning JNI init");

	window->application().init_jni();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	auto window = reinterpret_cast<SdlAppWindow*>(appstate);
	window->main_loop();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	
	auto window = reinterpret_cast<SdlAppWindow*>(appstate);
	window->handle_event(event);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	auto window = reinterpret_cast<SdlAppWindow*>(appstate);
	window->on_quit();
	
	delete window;
}

