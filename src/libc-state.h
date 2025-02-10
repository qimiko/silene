#pragma once

#ifndef _LIBC_STATE_H
#define _LIBC_STATE_H

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <list>

#include <spdlog/spdlog.h>

class PagedMemory;
class StateHolder;

class LibcState {
	struct StaticDestructor {
		std::uint32_t fn_ptr;
		std::uint32_t data;
	};

	PagedMemory& _memory;
	std::vector<StaticDestructor> _destructors{};

	std::unordered_map<std::string, std::string> _exposed_files{};
	std::unordered_map<std::uint32_t, std::FILE*> _open_files{};

	std::uint32_t _strtok_buffer{0u};

	struct MemoryChunk {
		std::uint32_t start_addr;
		std::uint32_t size;
		bool free;

		MemoryChunk* next{nullptr};
		MemoryChunk* prev{nullptr};
	};

	struct MemoryChunkComparator {
		bool operator()(const MemoryChunk* a, const MemoryChunk* b) const {
			return a->start_addr < b->start_addr;
		}
	};

	// O(1) insertion time
	MemoryChunk* _chunk_head{nullptr};
	MemoryChunk* _chunk_tail{nullptr};

	// provides O(1) access time
	std::unordered_map<std::uint32_t, MemoryChunk> _allocated_chunks{};

	std::mutex _allocator_lock{};

	void log_allocator_state();

	// this doesn't support multiple threads, too bad?
	std::uint32_t _errno_addr{0u};

public:

	std::uint32_t get_errno_addr() const;

	std::uint32_t allocate_memory(std::uint32_t size, bool zero_mem = false);
	void free_memory(std::uint32_t vaddr);
	std::uint32_t reallocate_memory(std::uint32_t vaddr, std::uint32_t size);

	void register_destructor(StaticDestructor destructor);

	void pre_init(const StateHolder& environment);

	std::uint32_t get_strtok_buffer() const;
	void set_strtok_buffer(std::uint32_t);

	std::uint32_t open_file(const std::string& filename, const char* mode);
	std::int32_t close_file(std::uint32_t file_ref);
	std::int32_t seek_file(std::uint32_t file_ref, std::int32_t offset, std::int32_t origin);
	std::int32_t read_file(void* buf, std::uint32_t size, std::uint32_t count, std::uint32_t file_ref);
	std::int32_t tell_file(std::uint32_t file_ref);

	void expose_file(std::string emu_name, std::string real_name);

	LibcState(PagedMemory& memory) : _memory(memory) {}
};

#endif
