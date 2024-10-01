#pragma once

#ifndef _LIBC_STATE_H
#define _LIBC_STATE_H

#include <cstdint>

#include <spdlog/spdlog.h>

#include "paged-memory.hpp"
#include "syscall-handler.hpp"
#include "syscall-translator.hpp"
#include "environment.h"

class LibcState {
	struct StaticDestructor {
		std::uint32_t fn_ptr;
		std::uint32_t data;
	};

	std::shared_ptr<PagedMemory> _memory{nullptr};
	std::vector<StaticDestructor> _destructors{};

	std::unordered_map<std::string, std::string> _exposed_files{};
	std::unordered_map<std::uint32_t, std::FILE*> _open_files{};

public:

	std::uint32_t allocate_memory(std::uint32_t size, bool zero_mem = false);
	void free_memory(std::uint32_t vaddr);
	std::uint32_t reallocate_memory(std::uint32_t vaddr, std::uint32_t size);

	void register_destructor(StaticDestructor destructor);

	void pre_init(Environment& environment);

	std::uint32_t open_file(const std::string& filename, const char* mode);
	std::int32_t close_file(std::uint32_t file_ref);
	std::int32_t seek_file(std::uint32_t file_ref, std::int32_t offset, std::int32_t origin);
	std::int32_t read_file(void* buf, std::uint32_t size, std::uint32_t count, std::uint32_t file_ref);
	std::int32_t tell_file(std::uint32_t file_ref);

	void expose_file(std::string emu_name, std::string real_name);

	LibcState(std::shared_ptr<PagedMemory> memory) : _memory(memory) {}
};

#endif
