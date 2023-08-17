#pragma once

#ifndef _ELF_H
#define _ELF_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <span>
#include <utility>

// os dependent calls...
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace Elf {

// todo: i hate the elf format
// this is all for 32bit format
// in the future i may expand to 64bit, but it's not worth it rn

enum class IdentifierClass : std::uint8_t {
	None = 0,
	Class32 = 1,
	Class64 = 2
};

enum class IdentifierData : std::uint8_t {
	None = 0,
	LittleEndian = 1,
	BigEndian = 2
};

// doesn't matter to identify the other types

enum class IdentifierAbi : std::uint8_t {
	None = 0,
	Linux = 3,
};

enum class Type : std::uint16_t {
	None = 0,
	Relocatable = 1,
	Executable = 2,
	Shared = 3,
	Core = 4
};

enum class Machine : std::uint16_t {
	None = 0,
	Armv7 = 40,
	AArch64 = 183,
};

// defines the header for a 32bit elf object
struct Header {
	struct {
		std::uint8_t magic[4]; // should = {0x7f, 'E', 'L', 'F'}
		IdentifierClass elf_class;
		IdentifierData data;
		std::uint8_t version; // should = 1
		IdentifierAbi abi; // should = AbiLinux
		std::uint8_t abi_version;
		std::uint8_t pad[7];
	} identifier; // should be 16 bits
	Type type;
	Machine machine;
	std::uint32_t version; // should also = 1
	std::uint32_t entry_point;
	std::uint32_t program_header_offset;
	std::uint32_t section_header_offset;
	std::uint32_t flags;
	std::uint16_t header_size;
	std::uint16_t program_header_entry_size;
	std::uint16_t program_header_entry_count;
	std::uint16_t section_header_entry_size;
	std::uint16_t section_header_entry_count;
	std::uint16_t section_header_string_table_idx;
};

enum class ProgramSegmentType : std::uint32_t {
	Null = 0,
	Load = 1,
	Dynamic = 2,
	Interpreter = 3,
	Note = 4,
	Shlib = 5,
	ProgramHeaderTable = 6,

	GnuStack = 1685382481, // segment contains the stack
	GnuRelro = 1685382482, // segment contains relocations and can be made read only
	ArmExceptionIdx = 1879048193, // ARM only exception stuff
};

// this is treated as bit flags, so enum class isn't appropriate
enum ProgramHeaderFlags : std::uint32_t {
	PF_Execute = 1,
	PF_Write = 2,
	PF_Read = 4,
};

struct ProgramHeader {
	ProgramSegmentType type;
	std::uint32_t segment_offset; // offset of segment in file
	std::uint32_t segment_virtual_address; // offset of segment after mapped to memory
	std::uint32_t segment_physical_address;
	std::uint32_t segment_file_size; // size of segment in file (in bytes)
	std::uint32_t segment_memory_size; // size of segment in memory (in bytes)
	ProgramHeaderFlags flags;
	std::uint32_t alignment; // 0/1: no alignment, virtual_addr = offset % alignment
};

enum class DynamicSectionTag : std::uint32_t {
	Null = 0,
	Needed = 1,
	ProcedureLinkageTableSize = 2,
	ProcedureLinkageTable = 3,
	Hash = 4,
	StringTable = 5,
	SymbolTable = 6,

	// entries have addend
	RelocationTable = 7,
	RelocationTableSize = 8,
	RelocationTableEntrySize = 9,

	StringTableSize = 10,
	SymbolTableEntrySize = 11,

	Init = 12,
	Fini = 13,
	SharedObjectName = 14,
	SearchPath = 15,
	Symbolic = 16,

	// entries have no addend (used on arm)
	RelTable = 17,
	RelTableSize = 18,
	RelTableEntrySize = 19,

	ProcedureLinkageTableRelocationEntryType = 20,
	Debug = 21,
	TextRelocation = 22,
	ProcedureLinkageRelocationsTable = 23,

	InitArray = 25,
	FiniArray = 26,
	InitArraySize = 27,
	FiniArraySize = 28
};

struct DynamicSectionEntry {
  DynamicSectionTag tag;
	std::uint32_t value;
};

// reference: https://github.com/ARM-software/abi-aa/blob/main/aaelf32/aaelf32.rst#5612relocation-types
// S - symbol address
// A - addend (not supported on older Android)
// T - thumb (1 if thumb, 0 if not)
// B(S) - idk
// these are for 32bit ARM only. just clarifying.
enum class RelocationType : std::uint8_t {
	Abs32 = 2, // (S + A) | T
	GlobalData = 21, // (S + A) | T
	JumpSlot = 22, // (S + A) | T
	Relative = 23, // B(S) + A
};

struct RelocationTableEntry {
	std::uint32_t offset;
	std::uint32_t info;

	std::uint32_t symbol() const { return this->info >> 8; }
  RelocationType type() const {
		return static_cast<RelocationType>(this->info & 0xff);
	}
};

enum class SymbolType {
	None = 0,
	Object = 1,
	Function = 2,
	Section = 3,
	File = 4,
	Common = 5,
	ThreadLocalStorage = 6
};

enum class SymbolBinding {
	Local = 0,
	Global = 1,
	Weak = 2
};

enum class SymbolVisibility {
	Default = 0,
	Internal = 1,
	Hidden = 2,
	Protected = 3
};

struct SymbolTableEntry {
	std::uint32_t name;
	std::uint32_t value;
	std::uint32_t size;
	std::uint8_t info;
	std::uint8_t other;
	std::uint16_t section_header_table_idx;

	SymbolType type() const { return static_cast<SymbolType>(this->info & 0xf); }
	SymbolBinding binding() const {
		return static_cast<SymbolBinding>(this->info >> 4);
	}

	SymbolVisibility visibility() const {
		return static_cast<SymbolVisibility>(this->info & 0x3);
	}
};

class File {
private:
	std::uint8_t* _mem{nullptr};
  std::int64_t _size{0};

	/**
	 * validates that the magic field of an elf is correct
	 */
	bool verify_elf() const;

public:
	std::uint8_t* memory() const;
	std::int64_t size() const;
	Header* header() const;

	/**
	 * returns the beginning of the program headers. can be indexed like an array
	 */
	std::span<ProgramHeader> program_headers() const;

	/**
	 * determines the loaded size of the executable
	 * @returns pair of (beginning offset, length)
	 */
	std::pair<std::size_t /* beginning */, std::size_t /* length */> load_size() const;

	File(const char* path);
	~File();
};

}

#endif