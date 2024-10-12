#include <filesystem>

#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>

#include "android-application.hpp"
#include "elf.h"
#include "android-window.h"
#include "zip-file.h"

extern "C" {
	void android_main(struct android_app* state);
}

void android_main(struct android_app* app) {
	auto android_logger = spdlog::android_logger_mt("android", "Silene");
	spdlog::set_default_logger(android_logger);

	// this is probably the cleanest option rn
	// but hopefully it can be improved
	std::filesystem::path internal_data{app->activity->internalDataPath};
	auto internal_apk = internal_data / "app.apk";
	std::string app_apk = internal_apk.string();

	AndroidApplication application{{false, app_apk}};

	AndroidWindow window{app, application};

	if (!window.init()) {
		spdlog::critical("failed to init window");
		return;
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
    std::terminate();
	}

	auto main_lib = apk_file.read_file_bytes(lib_path);
	auto elf = Elf::File(std::move(main_lib));

	auto asset_manager = app->activity->assetManager;
	auto zlib_asset = AAssetManager_open(asset_manager, "support/libz.so", AASSET_MODE_BUFFER);
	if (zlib_asset == nullptr) {
		spdlog::error("failed to open zlib from assets");
    std::terminate();
	}

	auto zlib_buffer = static_cast<const std::uint8_t*>(AAsset_getBuffer(zlib_asset));
	auto zlib_size = AAsset_getLength64(zlib_asset);

	std::vector<std::uint8_t> zlib_data{zlib_buffer, zlib_buffer + zlib_size};

	auto zlib = Elf::File(std::move(zlib_data));

	AAsset_close(zlib_asset);

	application.init();

	application.load_library(zlib);
	application.load_library(elf);

	application.finalize_libraries();

	spdlog::info("init fns done");

	application.init_jni();

	window.main_loop();
}