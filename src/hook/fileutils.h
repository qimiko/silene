#pragma once

#ifndef _HOOK_FILEUTILS_H
#define _HOOK_FILEUTILS_H

#include <filesystem>

#include "../environment.h"

std::uint32_t hook_CCFileUtils_getFileData(Environment& env, std::uint32_t class_ptr, std::uint32_t filename_ptr, std::uint32_t mode_ptr, std::uint32_t size_ptr) {
	std::string filename = env.memory_manager()->read_bytes<char>(filename_ptr);
	auto mode = env.memory_manager()->read_bytes<char>(mode_ptr);

	if (filename[0] != '/') {
		// relative to assets
		auto resource = std::filesystem::path(env.assets_dir()) / filename;
		if (resource.extension() == ".png") {
			// TODO: low res devices don't use hd, so this should be checked at runtime
			auto new_filename = fmt::format("{}-hd.png", resource.stem().string());
			auto hd_resource = resource;

			hd_resource.replace_filename(new_filename);
			if (std::filesystem::exists(hd_resource)) {
				resource = hd_resource;
			}
		}

		auto file = std::fopen(resource.string().c_str(), mode);
		if (!file) {
			return 0;
		}

		auto fsize = 0u;
		std::fseek(file, 0, SEEK_END);
		fsize = std::ftell(file);

		std::fseek(file, 0, SEEK_SET);

		auto out_loc = env.libc().allocate_memory(fsize, false);
		auto out_ptr = env.memory_manager()->read_bytes<unsigned char>(out_loc);

		fsize = std::fread(out_ptr, sizeof(unsigned char), fsize, file);
		std::fclose(file);

		if (size_ptr != 0) {
			env.memory_manager()->write_word(size_ptr, fsize);
		}

		return out_loc;
	}

	return 0;
}

#endif