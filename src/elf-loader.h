#pragma once

#ifndef _ELF_LOADER_H
#define _ELF_LOADER_H

#include <algorithm>
#include <memory>
#include <vector>

#include "paged-memory.hpp"
#include "elf.h"

namespace Elf {

class Loader {
private:
	/**
	 * contains all of the values from the dynamic section for easier reading
	 */
	struct DynamicInfo {
		std::uint32_t rel_table_offset{0};
		std::uint32_t rel_table_count{0};

		std::uint32_t plt_reloc_offset{0};
		std::uint32_t plt_reloc_count{0};

		std::uint32_t symbol_table_offset{0};
		std::uint32_t symbol_table_count{0};

		std::uint32_t string_table_offset{0};

		std::uint32_t init_array_offset{0};
		std::uint32_t init_array_count{0};
		std::uint32_t init_function_offset{0};

		std::uint32_t fini_array_offset{0};
		std::uint32_t fini_array_count{0};
		std::uint32_t fini_function_offset{0};
	};

	struct LoaderState {
		std::uint32_t base_address;
		std::uint32_t reloc_offset;
		DynamicInfo dynamic;
	};

	std::shared_ptr<PagedMemory> _memory{nullptr};
	std::uint32_t _load_addr{0};

	std::vector<std::uint32_t> _init_functions{};

	std::unordered_map<std::string, std::uint32_t> _loaded_symbols{};

	std::uint32_t _symbol_stub_addr{0xfdfdfdfd};

	std::unordered_map<std::string, std::uint32_t /* vaddr */> _symbol_stubs{};

	std::uint32_t resolve_sym_addr(const std::string_view& sym_name);

	/**
	 * performs the relocations that are specified in the relocations table at table_start.
	 */
	void relocate(const Elf::File& elf, LoaderState state, std::uint32_t table_offset, std::uint32_t count);

	void link(const Elf::File& elf, std::uint32_t base_addr, std::uint32_t reloc_offset, const std::span<DynamicSectionEntry>& dynamic_section);

public:
	std::uint32_t map_elf(const Elf::File& elf);

	const std::vector<std::uint32_t>& get_init_functions() const {
		return this->_init_functions;
	};

	const std::uint32_t get_symbol_addr(const std::string_view& symbol) const;
	bool has_symbol(const std::string_view& symbol) const;

	void set_symbol_fallback_addr(std::uint32_t vaddr);

	/**
	 * adds a stubbed symbol
	 * caller should set the last bit if it's a thumb instruction
	 */
	void add_stub_symbol(std::uint32_t vaddr, const std::string_view& symbol);

	Loader(std::shared_ptr<PagedMemory> memory) : _memory(memory) {}
};

}

#endif