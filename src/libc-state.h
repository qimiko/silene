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

public:

	std::uint32_t allocate_memory(std::uint32_t size, bool zero_mem = false);
	void free_memory(std::uint32_t vaddr);
	std::uint32_t reallocate_memory(std::uint32_t vaddr, std::uint32_t size);

	void register_destructor(StaticDestructor destructor);

	void pre_init(Environment& environment);

	LibcState(std::shared_ptr<PagedMemory> memory) : _memory(memory) {}
};

#endif
