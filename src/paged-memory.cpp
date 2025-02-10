#include "paged-memory.hpp"

std::uint8_t* PagedMemory::ptr_to_addr(std::uint32_t vaddr) {
	if (vaddr < EMU_PAGE_SIZE) [[unlikely]] {
		spdlog::warn("translating invalid address {:#x}", vaddr);
		throw std::runtime_error("null pointer exception!");
	}

	return _backing_memory + vaddr;
}

std::uint8_t PagedMemory::read_byte(std::uint32_t vaddr) {
	auto ret = this->ptr_to_addr(vaddr);
	return *ret;
}

std::uint16_t PagedMemory::read_halfword(std::uint32_t vaddr) {
	auto ret = this->ptr_to_addr(vaddr);
	// spdlog::trace("read half: {:#08x} = {:#04x}", vaddr, *ret);

	if (vaddr % 2 != 0) {
		std::uint16_t r_value;
		std::memcpy(&r_value, ret, sizeof(std::uint16_t));
		return r_value;
	}

	return *reinterpret_cast<std::uint16_t*>(ret);
}

std::uint32_t PagedMemory::read_word(std::uint32_t vaddr) {
	auto ret = this->ptr_to_addr(vaddr);
	// spdlog::trace("read word: {:#08x} = {:#08x}", vaddr, *ret);

	if (vaddr % 4 != 0) {
		std::uint32_t r_value;
		std::memcpy(&r_value, ret, sizeof(std::uint32_t));
		return r_value;
	}

	return *reinterpret_cast<std::uint32_t*>(ret);
}

std::uint64_t PagedMemory::read_doubleword(std::uint32_t vaddr) {
	auto ret = this->ptr_to_addr(vaddr);

	if (vaddr % 8 != 0) {
		std::uint64_t r_value;
		std::memcpy(&r_value, ret, sizeof(std::uint64_t));
		return r_value;
	}

	return *reinterpret_cast<std::uint64_t*>(ret);
}

void PagedMemory::write_byte(std::uint32_t vaddr, std::uint8_t value) {
	auto ptr = this->ptr_to_addr(vaddr);
	*ptr = value;
}

void PagedMemory::write_halfword(std::uint32_t vaddr, std::uint16_t value) {
	auto ptr = this->ptr_to_addr(vaddr);

	if (vaddr % 2 != 0) {
		std::memcpy(ptr, &value, sizeof(std::uint16_t));
	} else {
		*reinterpret_cast<std::uint16_t*>(ptr) = value;
	}
}

void PagedMemory::write_word(std::uint32_t vaddr, std::uint32_t value) {
	auto ptr = this->ptr_to_addr(vaddr);

	if (vaddr % 4 != 0) {
		std::memcpy(ptr, &value, sizeof(std::uint32_t));
	} else {
		*reinterpret_cast<std::uint32_t*>(ptr) = value;
	}
}

void PagedMemory::write_doubleword(std::uint32_t vaddr, std::uint64_t value) {
	auto ptr = this->ptr_to_addr(vaddr);

	if (vaddr % 8 != 0) {
		std::memcpy(ptr, &value, sizeof(std::uint64_t));
	} else {
		*reinterpret_cast<std::uint64_t*>(ptr) = value;
	}
}

void PagedMemory::allocate_stack(std::uint32_t stack_size) {
	this->_stack_min -= stack_size;
}

void PagedMemory::allocate(std::uint32_t bytes) {
	if (this->_max_addr > UINT32_MAX - bytes) {
		spdlog::error("memory has overrun the max size ({:#010x})", this->_max_addr);
	}

	if (this->_max_addr + bytes > this->_stack_min) {
		spdlog::warn("memory is beginning to overrun the stack ({:#010x})", this->_max_addr);
	}
	this->_max_addr += bytes;
};

std::uint32_t PagedMemory::get_next_addr() {
	return this->_max_addr;
}

std::uint32_t PagedMemory::get_next_word_addr() {
	if (this->_max_addr & 1) {
		// if the returned pointer is thumbed, increment by 1 to remove it
		this->_max_addr++;
	}

	return this->_max_addr;
}

std::uint32_t PagedMemory::get_next_page_aligned_addr() {
	auto current_page = this->_max_addr / EMU_PAGE_SIZE;
	auto offset = this->_max_addr % EMU_PAGE_SIZE;

	if (offset == 0) {
		// do not allocate, we ended on a page
		return this->_max_addr;
	}

	auto next_addr = EMU_PAGE_SIZE * (current_page + 1);
	this->_max_addr = next_addr;

	return next_addr;
}

void PagedMemory::copy(std::uint32_t vaddr, const void* src, std::uint32_t length) {
	std::memcpy(this->_backing_memory + vaddr, src, length);
}

void PagedMemory::set(std::uint32_t vaddr, std::uint8_t src, std::uint32_t length) {
	std::memset(this->_backing_memory + vaddr, src, length);
}

