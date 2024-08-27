#include <cstdint>
#include <cstdio>
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
#include "paged-memory.hpp"
#include "elf.h"
#include "elf-loader.h"

int main(int argc, char** argv) {
	CLI::App app{"App description"};
	argv = app.ensure_utf8(argv);

	std::string filename;
	app.add_option("binary", filename, "path to so file to load")
		->check(CLI::ExistingFile)
		->required();

	bool verbose = false;
	app.add_flag("-v,--verbose", verbose, "enable extra debug logging");

	bool enable_debugging = false;
	app.add_flag("-d,--debug", enable_debugging, "enables debugging through gdb on port 5039");

	CLI11_PARSE(app, argc, argv);

	if (verbose) {
		spdlog::set_level(spdlog::level::debug);
	}

	auto elf = Elf::File(filename);

	auto header = elf.header();

	if (
    header->type != Elf::Type::Shared ||
		header->machine != Elf::Machine::Armv7
	) {
		std::cout << "elf file does not match required parameters" << std::endl;
		return 1;
	}

/*
	for (const auto& segment : elf.program_headers()) {
		std::cout << "segment: type = " << static_cast<std::uint32_t>(segment.type) << " ";
		std::cout << "offset = 0x" << std::hex << segment.segment_offset << std::dec << " ";
		std::cout << "vaddr = 0x" << std::hex << segment.segment_virtual_address << std::dec << " ";
		std::cout << "addr = 0x" << std::hex << segment.segment_physical_address << std::dec << " ";
		std::cout << "filesz = 0x" << std::hex << segment.segment_file_size << std::dec << " ";
		std::cout << "memsz = 0x" << std::hex << segment.segment_memory_size << std::dec << " ";
		std::cout << "flags = " << segment.flags << " ";
		std::cout << "align = 0x" << std::hex << segment.alignment << std::dec;
		std::cout << std::endl;
	}
*/

	AndroidEnvironment env{};
	Dynarmic::A32::UserConfig user_config{};

	auto monitor = Dynarmic::ExclusiveMonitor{1};
	// this feels like a hack..?
	user_config.processor_id = 0;
	user_config.global_monitor = &monitor;

	// user_config.very_verbose_debugging_output = true;
	user_config.callbacks = &env;

	auto cpu = std::make_shared<Dynarmic::A32::Jit>(user_config);
	env.set_cpu(cpu);

	if (enable_debugging) {
		env.begin_debugging();
	}

	env.pre_init();

	env.program_loader().map_elf(elf);

	env.post_init();

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
	env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit", jni_env_ptr, 0, 1920, 1080);

	spdlog::info("r0: {:#08x}, pc: {:#08x}", cpu->Regs()[0], cpu->Regs()[15]);

	spdlog::info("done!");

	return 0;
}

