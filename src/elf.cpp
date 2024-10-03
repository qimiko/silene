#include "elf.h"

namespace Elf {

std::uint8_t* File::memory() const {
	return this->_mem;
}

std::int64_t File::size() const {
	return this->_size;
}

Header* File::header() const {
	return reinterpret_cast<Header*>(this->_mem);
}

bool File::verify_elf() const {
	auto header = this->header();
	auto magic = header->identifier.magic;
	// terrible way of expressing this but octal isn't very readable either
	if (memcmp(magic, "\x7F" "ELF", 4) != 0) {
		return false;
	}

	if (header->identifier.elf_class != IdentifierClass::Class32) {
		return false;
	}

	if (header->identifier.data != IdentifierData::LittleEndian) {
		return false;
	}

	if (header->version != 1) {
		return false;
	}

	if (header->header_size != sizeof(Header)) {
		throw std::logic_error("mapping mismatch: sizeof Header != header_size");
	}

	if (header->program_header_entry_size != sizeof(ProgramHeader)) {
		spdlog::error("ProgramHeader size mismatch. found: {}, exp: {}", header->program_header_entry_size, sizeof(ProgramHeader));
		throw std::logic_error("mapping mismatch: sizeof ProgramHeader != program_header_entry_size");
	}

	return true;
}

std::span<ProgramHeader> File::program_headers() const {
	auto header = this->header();
	auto offset = header->program_header_offset;

	auto header_begin = reinterpret_cast<ProgramHeader*>(this->memory() + offset);

	return { header_begin, header->program_header_entry_count };
}

std::pair<std::size_t, std::size_t> File::load_size() const {
	auto beginning = 0xFFFF'FFFFu; // an unsigned 32bit integer with max int value
	auto end = 0u;

	for (const auto& segment : this->program_headers()) {
		if (segment.type != ProgramSegmentType::Load) {
			// skip non load segments as they will not be mapped
			continue;
		}

		auto start_addr = segment.segment_virtual_address;
		auto length = segment.segment_memory_size;

		beginning = std::min(beginning, start_addr);
		end = std::max(end, start_addr + length);
	}

	return {beginning, end - beginning};
}

File::File(const std::string& path) {
	auto elf_fd = open(path.c_str(), O_RDONLY);

	struct stat s;

	if (fstat(elf_fd, &s) < 0) {
			close(elf_fd);
			throw std::runtime_error("failure to stat elf");
	}

	auto elf_size = s.st_size;

	auto elf_mem = mmap(nullptr, elf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, elf_fd, 0);
	if (elf_mem == MAP_FAILED) {
			close(elf_fd);
			throw std::runtime_error("elf mapping failure");
	}

	close(elf_fd);

	this->_mem = reinterpret_cast<std::uint8_t*>(elf_mem);
	this->_size = elf_size;
	this->_mmapped_data = true;

	if (!this->verify_elf()) {
			throw std::runtime_error("failure to verify elf magic");
	}
}

File::File(std::vector<std::uint8_t>&& elf_mem) {
	// take ownership of the vector to prevent lifetime issues
	this->_in_mem_data = std::move(elf_mem);
	this->_mem = _in_mem_data.data();
	this->_size = _in_mem_data.size();
	this->_mmapped_data = false;

	if (this->_mem == nullptr) {
		throw std::runtime_error("elf mem is nullptr");
	}

	if (!this->verify_elf()) {
			throw std::runtime_error("failure to verify elf magic");
	}
}

File::~File() {
	if (this->_mmapped_data) {
		munmap(this->_mem, this->_size);
	}

	this->_mem = nullptr;
	this->_size = 0;
}

}