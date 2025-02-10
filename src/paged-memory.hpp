#pragma once

#ifndef _PAGED_MEMORY_H
#define _PAGED_MEMORY_H

#include <array>
#include <cstdint>
#include <unordered_map>

#include <sys/mman.h>

#include <spdlog/spdlog.h>

class PagedMemory {
private:
	static constexpr std::uint32_t STACK_SIZE = 1024 * 1024;

	// have a backing 4gb set of continuous memory
	// after looking into it, it seems like lazy allocation should take over
	// as long as we don't use std::array...
	static constexpr std::uint32_t MEMORY_MAX = 0xffff'ffff; // should correspond to 4 GB
	std::uint8_t* _backing_memory{nullptr};

	std::uint32_t _max_addr{0};
	std::uint32_t _stack_min{MEMORY_MAX};

	std::uint8_t* ptr_to_addr(std::uint32_t vaddr);

public:
	static constexpr std::uint32_t EMU_PAGE_SIZE = 4 * 1024;

	PagedMemory() {
		this->_backing_memory = reinterpret_cast<std::uint8_t*>(mmap(
			nullptr,
			MEMORY_MAX,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0
		));

		spdlog::info("loading memory at real addr {:#x}",
			reinterpret_cast<std::uint64_t>(this->_backing_memory)
		);

		if (this->_backing_memory == MAP_FAILED) {
			spdlog::error("main memory allocation failed, {}", errno);
			throw new std::runtime_error("memory allocation failed :(");
		}
	}

	PagedMemory(const PagedMemory&) = delete;
	PagedMemory& operator=(const PagedMemory&) = delete;

	~PagedMemory() {
		munmap(this->_backing_memory, MEMORY_MAX);
	}

	/**
	 * reads an 8 bit value at the specified address
	 */
	std::uint8_t read_byte(std::uint32_t vaddr);

	// these sized functions are probably not sustainable, may be good to make templated

	/**
	 * reads a 16 bit value at the specified address
	 */
	std::uint16_t read_halfword(std::uint32_t vaddr);

	/**
	 * reads a 32 bit value at the specified address
	 */
	std::uint32_t read_word(std::uint32_t vaddr);

	/**
	 * reads a 64 bit value at the specified address
	 */
	std::uint64_t read_doubleword(std::uint32_t vaddr);

	/**
	 * writes an 8 bit value at the specified address
	 */
	void write_byte(std::uint32_t vaddr, std::uint8_t value);

	/**
	 * writes a 16 bit value at the specified address
	 */
	void write_halfword(std::uint32_t vaddr, std::uint16_t value);

	/**
	 * writes a 32 bit value at the specified address
	 */
	void write_word(std::uint32_t vaddr, std::uint32_t value);

	/**
	 * writes a 64 bit value at the specified address
	 */
	void write_doubleword(std::uint32_t vaddr, std::uint64_t value);

	template <typename T>
	T* read_bytes(std::uint32_t vaddr) {
		auto ret = reinterpret_cast<T*>(this->ptr_to_addr(vaddr));
		return ret;
	}

	/**
	 * marks the stack portion of memory as valid
	 * (from the end of memory to stack size)
	 */
	void allocate_stack(std::uint32_t stack_size = STACK_SIZE);

	/**
	 * marks some portion of memory as allocated
	 */
	void allocate(std::uint32_t bytes);

	/**
	 * gets the next available address
	 * makes no promises about allocation.
	 */
	std::uint32_t get_next_addr();

	/**
	 * gets the next available word aligned address
	 * allocates up to this pointer
	 */
	std::uint32_t get_next_word_addr();

	/**
	 * gets the next available page, based on EMU_PAGE_SIZE.
	 * also should allocate up to this pointer
	 */
	std::uint32_t get_next_page_aligned_addr();

	void copy(std::uint32_t vaddr, const void* src, std::uint32_t length);
	void set(std::uint32_t vaddr, std::uint8_t value, std::uint32_t length);
};

#endif
