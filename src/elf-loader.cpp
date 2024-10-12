#include "elf-loader.h"

#include <spdlog/fmt/ranges.h>

std::uint32_t Elf::Loader::resolve_sym_addr(const std::string_view& sym_name) {
	auto symbol_str = std::string(sym_name);

	// symbol stubs take precedence over other symbols
	if (auto it = this->_symbol_stubs.find(symbol_str); it != this->_symbol_stubs.end()) {
		return it->second;
	}

	// otherwise try to resolve with native ones
	if (auto it = this->_loaded_symbols.find(symbol_str); it != this->_loaded_symbols.end()) {
		return it->second;
	}

	return 0;
}

void Elf::Loader::relocate(const Elf::File& elf, LoaderState state, std::uint32_t table_offset, std::uint32_t count) {
	auto rel_size = sizeof(RelocationTableEntry);

	auto addr = state.base_address + table_offset;
	auto vaddr = this->_memory.read_bytes<RelocationTableEntry>(addr);
	auto reloc_array = std::span{vaddr, count};

	for (const auto& reloc : reloc_array) {
		auto symbol_idx = reloc.symbol();

		spdlog::debug("rel at {:#08x}: to - {:#08x} symbol - {:#08x} type - {:#x}",
			addr,
			state.base_address + reloc.offset,
			symbol_idx,
			static_cast<std::uint8_t>(reloc.type())
		);

		// resolve symbol
		// note: for data relocations this will require actually resolving
		// figure out what to do when that becomes necessary...
		auto sym_addr = this->_symbol_stub_addr;

		if (symbol_idx != 0) {
			// state.dynamic.string_table_offset;
			auto symbol_table_addr = state.base_address + state.dynamic.symbol_table_offset;
			auto symbol_table = this->_memory.read_bytes<SymbolTableEntry>(symbol_table_addr);
			auto symbol = symbol_table + symbol_idx;

			auto name_offset = symbol->name;
			auto string_table_addr = state.base_address + state.dynamic.string_table_offset;

			auto name_str = this->_memory.read_bytes<char>(string_table_addr + name_offset);

			auto symbol_name = std::string(name_str);
			spdlog::debug("resolve symbol {}", symbol_name);

			auto addr = this->resolve_sym_addr(symbol_name);
			if (addr) {
				// fn returns 0 on failure, don't accept that
				sym_addr = addr;
			} else {
				spdlog::warn("missing symbol {}", symbol_name, symbol->info);
			}

			// state.dynamic.symbol_table_offset;
		} else {
			sym_addr = 0;
		}

		auto reloc_addr = state.base_address + reloc.offset;

		switch (reloc.type()) {
			case RelocationType::Abs32: {
				auto orig_addr = this->_memory.read_word(reloc_addr);
				auto offset_addr = orig_addr + sym_addr;
				this->_memory.write_word(reloc_addr, offset_addr);
				break;
			}
			case RelocationType::GlobalData:
				// will have to determine if something special needs to be done
				[[fallthrough]];
			case RelocationType::JumpSlot: {
				this->_memory.write_word(reloc_addr, sym_addr);
				break;
			}
			case RelocationType::Relative: {
				auto orig_addr = this->_memory.read_word(reloc_addr);
				auto offset_addr = orig_addr + state.base_address;
				this->_memory.write_word(reloc_addr, offset_addr);
				break;
			}
			default:
				spdlog::warn("unrecognized relocation type {:#x}",
					static_cast<std::uint8_t>(reloc.type())
				);
		}

		addr += rel_size;
	}
}

void Elf::Loader::link(const Elf::File& elf, std::uint32_t base_addr, std::uint32_t reloc_offset, const std::span<DynamicSectionEntry>& dynamic_section) {
	// parse necessary details
	// not a big fan of this but it'll do.. for now

	DynamicInfo info = {};

	for (const auto& dynamic : dynamic_section) {
		if (dynamic.tag == DynamicSectionTag::Null) {
			// end if null tag is found (it marks the end)
			break;
		}

		spdlog::trace("dynamic: {:x} {:#08x}",
			static_cast<std::uint32_t>(dynamic.tag),
			dynamic.value
		);

		switch (dynamic.tag) {
			case DynamicSectionTag::RelTable:
				info.rel_table_offset = dynamic.value;
				break;
			case DynamicSectionTag::RelTableSize:
				info.rel_table_count = dynamic.value / sizeof(RelocationTableEntry);
				break;
			case DynamicSectionTag::RelTableEntrySize:
				if (dynamic.value != sizeof(RelocationTableEntry)) {
					throw std::logic_error("mapping mismatch: sizeof RelTableEntry != rel_table_entry_size");
				}
				break;
			case DynamicSectionTag::ProcedureLinkageTableRelocationEntryType:
				if (static_cast<DynamicSectionTag>(dynamic.value) != DynamicSectionTag::RelTable) {
					throw std::runtime_error("plt type mismatch: expected DynamicSectionTag::RelTable");
				}
				break;
			case DynamicSectionTag::ProcedureLinkageTableSize:
				info.plt_reloc_count = dynamic.value / sizeof(RelocationTableEntry);
				break;
			case DynamicSectionTag::ProcedureLinkageRelocationsTable:
				info.plt_reloc_offset = dynamic.value;
				break;
			case DynamicSectionTag::ProcedureLinkageTable:
				// no need to store the address, so ignore
				break;
			case DynamicSectionTag::RelocationTable:
			case DynamicSectionTag::RelocationTableSize:
			case DynamicSectionTag::RelocationTableEntrySize:
				// android linker only deals with rela sections
				spdlog::warn("found unexpected relocation table in dynamic section");
				break;
			case DynamicSectionTag::Hash: {
				// we will write our own hashing routine
				// no need for the elf hash table
				auto nchain = this->_memory.read_word(base_addr + dynamic.value + 4);
				info.symbol_table_count = nchain;
				break;
			}
			case DynamicSectionTag::SymbolTable:
				info.symbol_table_offset = dynamic.value;
				break;
			case DynamicSectionTag::SymbolTableEntrySize:
				if (dynamic.value != sizeof(SymbolTableEntry)) {
					throw std::logic_error("mapping mismatch: sizeof SymbolTableEntry != symbol_table_entry_size");
				}
				break;
			case DynamicSectionTag::StringTable:
				info.string_table_offset = dynamic.value;
			case DynamicSectionTag::StringTableSize:
				// no need to store this, so ignore as well
				break;
			case DynamicSectionTag::Init:
				info.init_function_offset = dynamic.value;
				break;
			case DynamicSectionTag::InitArray:
				info.init_array_offset = dynamic.value;
				break;
			case DynamicSectionTag::InitArraySize:
				info.init_array_count = dynamic.value / 4; // sizeof(std::uint32_t)
				break;
			case DynamicSectionTag::Fini:
				info.fini_function_offset = dynamic.value;
				break;
			case DynamicSectionTag::FiniArray:
				info.fini_array_offset = dynamic.value;
				break;
			case DynamicSectionTag::FiniArraySize:
				info.fini_array_count = dynamic.value / 4;
				break;
			default:
				spdlog::warn("unrecognized dynamic entry: {:#x}",
					static_cast<std::uint32_t>(dynamic.tag)
				);
				continue;
		}
	}

	// todo: look into loading dependencies when needed

	LoaderState state = {
		base_addr, reloc_offset, info
	};

	// load addresses into global symbol table
	// this happens first, as some relocations may depend on the existence of a symbol in the binary

	auto string_table_addr = state.base_address + state.dynamic.string_table_offset;

	auto symbol_table_addr = base_addr + info.symbol_table_offset;
	auto symbol_table_ptr = this->_memory.read_bytes<SymbolTableEntry>(symbol_table_addr);
	auto symbol_table = std::span{symbol_table_ptr, info.symbol_table_count};

	for (const auto& symbol : symbol_table) {
		if (
			symbol.name == 0 ||
			symbol.value == 0 ||
			symbol.type() != SymbolType::Function
		) {
			// no need to load non functions/empty symbols
			continue;
		}

		auto name_offset = symbol.name;
		auto name_str = this->_memory.read_bytes<char>(string_table_addr + name_offset);
		auto symbol_name = std::string(name_str);

		spdlog::trace("symbol {}: visibility - {:#x}, type - {:#x}, binding - {:#x}, size - {:#x}, value - {:#08x}, shndx - {:#x}",
			symbol_name,
			static_cast<std::uint8_t>(symbol.visibility()),
			static_cast<std::uint8_t>(symbol.type()),
			static_cast<std::uint8_t>(symbol.binding()),
			symbol.size,
			base_addr + symbol.value,
			symbol.section_header_table_idx
		);

		this->_loaded_symbols.insert({symbol_name, base_addr + symbol.value});
	}

	// perform relocations

	if (info.plt_reloc_offset != 0) {
		spdlog::info("performing plt relocations: {:#08x}", info.plt_reloc_offset);
		this->relocate(elf, state, info.plt_reloc_offset, info.plt_reloc_count);
	}

	if (info.rel_table_offset != 0) {
		spdlog::info("performing relocations: {:#08x}", info.rel_table_offset);
		this->relocate(elf, state, info.rel_table_offset, info.rel_table_count);
	}

	spdlog::info("relocations done!");

	// add functions into init/fini arrays
	// dt_init first, dt_init_array functions next

	if (info.init_function_offset != 0) {
		auto init_function = info.init_function_offset;

		this->_init_functions.push_back(init_function);
	}

	if (info.init_array_offset != 0 && info.init_array_count != 0) {
		auto init_array = base_addr + info.init_array_offset;
		auto init_size = info.init_array_count;
		auto fn_size = 4;

		spdlog::trace("loading init array: {:#08x} {:#x}", init_array, init_size);

		auto init_start = this->_memory.read_bytes<std::uint32_t>(init_array);
		auto init_fns = std::span{init_start, init_size};

		for (const auto& fn_addr : init_fns) {
			this->_init_functions.push_back(fn_addr);

			init_array += fn_size;
		}

	}

	/*
	// write this one day... currently not necessary
	if (info.fini_function_offset != 0) {
		auto fini_function = info.fini_function_offset;
	}

	if (info.fini_array_offset != 0 && info.fini_array_count != 0) {
		auto fini_array = base_addr + info.fini_array_offset;
		auto fini_size = info.fini_array_count;
	}
	*/
}

std::uint32_t Elf::Loader::map_elf(const Elf::File& elf) {
	// begin the fun process of copying over memory
	auto file_mem = elf.memory();

	// for shared objects, this will be where memory starts
	// for executables, this should be null (as they are the first thing loaded)
	auto load_bias = this->_memory.get_next_page_aligned_addr();
	auto reloc_offset = 0u;

	// in ghidra, this ends up being 0x10000 - bias = 0xf000
	spdlog::info("loading object with bias {:#08x}", load_bias);
	this->_load_addr = load_bias;

	std::span<Elf::DynamicSectionEntry> dynamic_segment{};
	for (const auto& segment : elf.program_headers()) {
		if (segment.type == ProgramSegmentType::ProgramHeaderTable) {
			reloc_offset = segment.segment_offset;
		}

		if (segment.type == ProgramSegmentType::Dynamic) {
			auto entry_count = segment.segment_file_size / sizeof(DynamicSectionEntry);
			auto dynamic = reinterpret_cast<DynamicSectionEntry*>(file_mem + segment.segment_offset);

			dynamic_segment = {dynamic, entry_count};
		}

		if (segment.type != ProgramSegmentType::Load) {
			// skip non load segments
			continue;
		}

		auto start_offset = segment.segment_offset;
		auto length = segment.segment_file_size;

		auto start_addr = load_bias + segment.segment_virtual_address;

		// keep our bss safe
		this->_memory.allocate(segment.segment_memory_size);

		spdlog::trace("copying segment from file+{:#x} to {:#08x}", start_offset, start_addr);
		this->_memory.copy(start_addr, file_mem + start_offset, length);
	}

	this->link(elf, load_bias, reloc_offset, dynamic_segment);

	return load_bias + elf.header()->entry_point;
}

void Elf::Loader::set_symbol_fallback_addr(std::uint32_t vaddr) {
	this->_symbol_stub_addr = vaddr;
}

void Elf::Loader::add_stub_symbol(std::uint32_t vaddr, const std::string_view& symbol) {
	auto sym_str = std::string(symbol);
	this->_symbol_stubs[sym_str] = vaddr;

	spdlog::info("adding stub symbol {} at {:#08x}", symbol, vaddr);
}

std::uint32_t Elf::Loader::get_symbol_addr(const std::string_view& symbol) const {
	auto symbol_str = std::string(symbol);
	if (auto it = this->_loaded_symbols.find(symbol_str); it != this->_loaded_symbols.end()) {
		return it->second;
	}

	return 0;
}

bool Elf::Loader::has_symbol(const std::string_view& symbol) const {
	auto symbol_str = std::string(symbol);
	return this->_loaded_symbols.contains(symbol_str);
}
