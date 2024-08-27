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

#include <spdlog/spdlog.h>

#include "android-environment.hpp"
#include "paged-memory.hpp"
#include "elf.h"
#include "elf-loader.h"

struct Args {
	bool verbose{false};
	bool enable_debugger{false};
	std::string filename{};
	std::string pathname{};
};

Args parse_cli(std::span<char*> args) {
	Args arg_obj{};

	for (const auto& arg : args) {
		if (std::strcmp(arg, "--verbose") == 0 || std::strcmp(arg, "-v") == 0) {
			arg_obj.verbose = true;
		} else if (std::strcmp(arg, "--debug") == 0) {
			arg_obj.enable_debugger = true;
		} else if (arg_obj.pathname.empty()) {
			arg_obj.pathname = arg;
		} else if (arg_obj.filename.empty()) {
			arg_obj.filename = arg;
		}
	}

	return arg_obj;
}

int main(int argc, char** argv) {
	auto args = parse_cli({
		argv,
		static_cast<std::size_t>(argc)
	});

	if (args.filename.empty()) {
		std::cout << "usage: " << args.pathname << " <so file> [-v|--verbose] [--debug]" << std::endl;
		return 1;
	}

	if (args.verbose) {
		spdlog::set_level(spdlog::level::debug);
	}

	auto elf = Elf::File(args.filename);

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

	if (args.enable_debugger) {
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

