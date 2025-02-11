#include "libc-state.h"

#include <sstream>

#include "syscall-handler.hpp"
#include "syscall-translator.hpp"

#include "libc/cxa.h"
#include "libc/math.h"
#include "libc/string.h"
#include "libc/wchar.h"
#include "libc/stdlib.h"
#include "libc/pthread.h"
#include "libc/locale.h"
#include "libc/stdio.h"
#include "libc/socket.h"
#include "libc/wctype.h"
#include "libc/log.h"
#include "libc/setjmp.h"
#include "libc/time.h"
#include "libc/ctype.h"
#include "libc/semaphore.h"
#include "libc/unistd.h"
#include "libc/inet.h"
#include "libc/signal.h"
#include "libc/poll.h"
#include "libc/errno.h"

#include "kernel/kernel.h"

#include "gl/gl-wrap.h"

void LibcState::log_allocator_state() {
	std::stringstream ss;

	auto chunk = _chunk_head;
	while (chunk) {
		ss << "[";
		if (chunk->free) {
			ss << "*";
		}
		ss << std::hex << chunk->start_addr << " size=" << chunk->size << "]=>";
		chunk = chunk->next;
	}

	ss << _chunk_tail->start_addr;

	spdlog::info("alloc state: {}", ss.str());
}

std::uint32_t LibcState::allocate_memory(std::uint32_t size, bool zero_mem) {
	// TODO: this implementation is terrible.
	if (size == 0) {
		return 0;
	}

	// bump size to nearest dword
	size += 8 - (size % 8);

	std::scoped_lock lk{_allocator_lock};

	auto first_free = _chunk_head;
	while (first_free) {
		if (first_free->free && first_free->size >= size) {
			break;
		}

		first_free = first_free->next;
	}

	if (first_free == nullptr) {
		// we can just allocate some large chunk idk
		auto next = this->_memory.get_next_page_aligned_addr();

		auto base_heap_size = std::max(0x1000000u, size);
		this->_memory.allocate(base_heap_size); // i think this is 16mb

		_allocated_chunks.try_emplace(next, next, base_heap_size, true, nullptr, _chunk_tail);
		auto allocated = &_allocated_chunks.at(next);

		if (!_chunk_head) {
			_chunk_head = allocated;
		} else {
			_chunk_tail->next = allocated;
		}

		_chunk_tail = allocated;
		first_free = allocated;
	}

	if (first_free->size > size) {
		// split the chunk
		auto end_ptr = first_free->start_addr + size;
		auto next_chunk_size = first_free->size - size;

		_allocated_chunks.try_emplace(end_ptr, end_ptr, next_chunk_size, true, first_free->next, first_free);
		auto new_chunk = &_allocated_chunks.at(end_ptr);

		if (first_free->next) {
			first_free->next->prev = new_chunk;
		}

		first_free->size = size;
		first_free->next = new_chunk;
	}

	auto start = first_free->start_addr;
	if (zero_mem) {
		this->_memory.set(start, 0, size);
	}

	first_free->free = false;

	return start;
}

void LibcState::free_memory(std::uint32_t vaddr) {
	if (vaddr == 0) {
		return;
	}

	if (!_allocated_chunks.contains(vaddr)) {
		spdlog::warn("attempted to free unallocated chunk at {:#010x}", vaddr);
		throw std::runtime_error("double free detected");

		return;
	}

	std::scoped_lock lk{_allocator_lock};

	auto chunk = &_allocated_chunks.at(vaddr);
	chunk->free = true;

	// merge with the next chunk, if available
	if (chunk->next && chunk->next->free) {
		auto next_chunk = chunk->next;

		if (next_chunk->next) {
			next_chunk->next->prev = chunk;
		}

		if (_chunk_tail == next_chunk) {
			_chunk_tail = chunk;
		}

		chunk->size += next_chunk->size;
		chunk->next = next_chunk->next;

		_allocated_chunks.erase(next_chunk->start_addr);
	}

	// in this case, we delete our chunk and move it into the previous
	if (chunk->prev && chunk->prev->free) {
		auto prev_chunk = chunk->prev;
		prev_chunk->size += chunk->size;

		prev_chunk->next = chunk->next;

		if (chunk->next) {
			chunk->next->prev = prev_chunk;
		}

		if (_chunk_tail == chunk) {
			_chunk_tail = prev_chunk;
		}

		_allocated_chunks.erase(chunk->start_addr);
	}
}

std::uint32_t LibcState::reallocate_memory(std::uint32_t vaddr, std::uint32_t size) {
	if (vaddr == 0 || !_allocated_chunks.contains(vaddr)) {
		return this->allocate_memory(size);
	}

	if (size == 0) {
		this->free_memory(vaddr);
		return 0;
	}

	size += 8 - (size % 8);

	auto chunk = &_allocated_chunks.at(vaddr);
	if (chunk->size == size) {
		return vaddr;
	}

	if (!chunk->next || !chunk->next->free || chunk->size + chunk->next->size < size) {
		auto new_ptr = this->allocate_memory(size, false);

		auto src = this->_memory.read_bytes<std::uint8_t>(vaddr);
		auto dest = this->_memory.read_bytes<std::uint8_t>(new_ptr);

		std::memcpy(dest, src, chunk->size);

		this->free_memory(vaddr);

		return new_ptr;
	}

	// we have enough space to move into the next chunk (no copy)

	// merge next chunk into our chunk
	auto next_chunk = chunk->next;

	if (next_chunk->next) {
		next_chunk->next->prev = chunk;
	}

	if (_chunk_tail == next_chunk) {
		_chunk_tail = chunk;
	}

	chunk->size += next_chunk->size;
	chunk->next = next_chunk->next;

	_allocated_chunks.erase(next_chunk->start_addr);

	// split current chunk into something more appropriately sized
	if (chunk->size > size) {
		auto end_ptr = chunk->start_addr + size;
		auto next_chunk_size = chunk->size - size;

		_allocated_chunks.try_emplace(end_ptr, end_ptr, next_chunk_size, true, chunk->next, chunk);
		auto new_chunk = &_allocated_chunks.at(end_ptr);

		if (chunk->next) {
			chunk->next->prev = new_chunk;
		}

		chunk->size = size;
		chunk->next = new_chunk;
	}

	return vaddr;
}

void LibcState::register_destructor(StaticDestructor destructor) {
	this->_destructors.push_back(destructor);
}

std::uint32_t LibcState::open_file(const std::string& filename, const char* mode) {
	auto it = _exposed_files.find(filename);
	if (it == _exposed_files.end()) {
		spdlog::warn("tried to open unknown file: {}", filename);
		return 0;
	}

	auto& exposed_name = it->second;

	auto file_ptr = std::fopen(exposed_name.c_str(), mode);
	if (!file_ptr) {
		return 0;
	}

	auto addr = this->allocate_memory(4);

	// write itself for safekeeping
	this->_memory.write_word(addr, addr);

	_open_files[addr] = file_ptr;
	return addr;
}

std::int32_t LibcState::close_file(std::uint32_t file_ref) {
	auto it = _open_files.find(file_ref);
	if (it == _open_files.end()) {
		spdlog::warn("tried to close unknown file {:#x}", file_ref);
		return -1;
	}

	auto file_ptr = it->second;

	auto r = fclose(file_ptr);

	this->free_memory(file_ref);
	_open_files.erase(it);

	return r;
}

std::int32_t LibcState::seek_file(std::uint32_t file_ref, std::int32_t offset, std::int32_t origin) {
	auto it = _open_files.find(file_ref);
	if (it == _open_files.end()) {
		spdlog::warn("tried to seek unknown file {:#x}", file_ref);
		return -1;
	}

	auto file_ptr = it->second;
	return std::fseek(file_ptr, offset, origin);
}

std::int32_t LibcState::read_file(void* buf, std::uint32_t size, std::uint32_t count, std::uint32_t file_ref) {
	auto it = _open_files.find(file_ref);
	if (it == _open_files.end()) {
		spdlog::warn("tried to read unknown file {:#x}", file_ref);
		return 0;
	}

	auto file_ptr = it->second;

	return static_cast<std::uint32_t>(std::fread(buf, size, count, file_ptr));
}

std::int32_t LibcState::tell_file(std::uint32_t file_ref) {
	auto it = _open_files.find(file_ref);
	if (it == _open_files.end()) {
		spdlog::warn("tried to tell unknown file {:#x}", file_ref);
		return -1;
	}
	auto file_ptr = it->second;

	return static_cast<std::uint32_t>(std::ftell(file_ptr));
}

void LibcState::expose_file(std::string emu_name, std::string real_name) {
	_exposed_files[emu_name] = real_name;
}

std::uint32_t LibcState::get_strtok_buffer() const {
	return this->_strtok_buffer;
}

void LibcState::set_strtok_buffer(std::uint32_t x) {
	this->_strtok_buffer = x;
}

std::uint32_t LibcState::get_errno_addr() const {
	return this->_errno_addr;
}

void LibcState::pre_init(const StateHolder& env) {
	REGISTER_FN(env, sin);
	REGISTER_FN(env, sinf);
	REGISTER_FN(env, cos);
	REGISTER_FN(env, cosf);
	REGISTER_FN(env, sqrt);
	REGISTER_FN(env, sqrtf);
	REGISTER_FN(env, ceilf);
	REGISTER_FN(env, ceil);
	REGISTER_FN(env, round);
	REGISTER_FN(env, roundf);
	REGISTER_FN(env, lroundf);
	REGISTER_FN(env, atan);
	REGISTER_FN(env, atan2);
	REGISTER_FN(env, atan2f);
	REGISTER_FN(env, acos);
	REGISTER_FN(env, acosf);
	REGISTER_FN(env, floor);
	REGISTER_FN(env, floorf);
	REGISTER_FN(env, powf);
	REGISTER_FN(env, pow);
	REGISTER_FN(env, fmodf);
	REGISTER_FN(env, fmod);
	REGISTER_FN(env, asinf);
	REGISTER_FN(env, asinh);
	REGISTER_FN(env, asinhf);
	REGISTER_FN(env, tanf);
	REGISTER_FN(env, tanh);
	REGISTER_FN(env, tanhf);
	REGISTER_FN(env, __fpclassifyd);
	REGISTER_FN(env, __cxa_atexit);
	REGISTER_FN(env, __cxa_finalize);
	REGISTER_FN(env, __gnu_Unwind_Find_exidx);
	REGISTER_FN(env, strcmp);
	REGISTER_FN(env, strncmp);
	REGISTER_FN(env, btowc);
	REGISTER_FN(env, wctype);
	REGISTER_FN(env, wcslen);
	REGISTER_FN(env, wctob);
	REGISTER_FN(env, atoi);
	REGISTER_FN(env, atof);
	REGISTER_FN(env, pthread_key_create);
	REGISTER_FN(env, pthread_key_delete);
	REGISTER_FN(env, pthread_once);
	REGISTER_FN(env, pthread_create);
	REGISTER_FN(env, pthread_detach);
	REGISTER_FN(env, pthread_mutex_init);
	REGISTER_FN(env, pthread_mutex_lock);
	REGISTER_FN(env, pthread_mutex_destroy);
	REGISTER_FN(env, pthread_mutex_unlock);
	REGISTER_FN(env, pthread_cond_broadcast);
	REGISTER_FN(env, pthread_cond_wait);
	REGISTER_FN(env, pthread_getspecific);
	REGISTER_FN(env, pthread_setspecific);
	REGISTER_FN(env, pthread_exit);
	REGISTER_FN(env, sem_init);
	REGISTER_FN(env, sem_post);
	REGISTER_FN(env, sem_wait);
	REGISTER_FN(env, sem_destroy);
	REGISTER_FN(env, memcpy);
	REGISTER_FN(env, memmove);
	REGISTER_FN(env, memchr);
	REGISTER_FN(env, malloc);
	REGISTER_FN(env, free);
	REGISTER_FN(env, realloc);
	REGISTER_FN(env, calloc);
	REGISTER_FN(env, abort);
	REGISTER_FN(env, strlen);
	REGISTER_FN(env, memset);
	REGISTER_FN(env, setlocale);
	REGISTER_FN(env, fopen);
	REGISTER_FN(env, fclose);
	REGISTER_FN(env, fseek);
	REGISTER_FN(env, ftell);
	REGISTER_FN(env, fread);
	REGISTER_FN(env, fwrite);
	REGISTER_FN(env, fputs);
	REGISTER_FN(env, getsockopt);
	REGISTER_FN(env, connect);
	REGISTER_FN(env, getpeername);
	REGISTER_FN(env, getsockname);
	REGISTER_FN(env, socket);
	REGISTER_FN(env, send);
	REGISTER_FN(env, __stack_chk_fail);
	REGISTER_FN(env, memcmp);
	REGISTER_FN(env, __android_log_print);
	REGISTER_FN(env, sprintf);
	REGISTER_FN(env, snprintf);
	REGISTER_FN(env, strcpy);
	REGISTER_FN(env, strncpy);
	REGISTER_FN(env, vsprintf);
	REGISTER_FN(env, vsnprintf);
	REGISTER_FN(env, sscanf);
	REGISTER_FN(env, setjmp);
	REGISTER_FN(env, longjmp);
	REGISTER_FN(env, sigsetjmp);
	REGISTER_FN(env, fprintf);
	REGISTER_FN(env, fputc);
	REGISTER_FN(env, strtol);
	REGISTER_FN(env, strtoll);
	REGISTER_FN(env, strtoul);
	REGISTER_FN(env, strtod);
	REGISTER_FN(env, strstr);
	REGISTER_FN(env, strtok);
	REGISTER_FN(env, strdup);
	REGISTER_FN(env, strchr);
	REGISTER_FN(env, strrchr);
	REGISTER_FN(env, strerror_r);
	REGISTER_FN(env, gettimeofday);
	REGISTER_FN(env, time);
	REGISTER_FN(env, clock_gettime);
	REGISTER_FN(env, localtime);
	REGISTER_FN(env, getenv);
	REGISTER_FN(env, lrand48);
	REGISTER_FN(env, arc4random);
	REGISTER_FN(env, ftime);
	REGISTER_FN(env, srand48);
	REGISTER_FN(env, qsort);
	REGISTER_FN(env, tolower);
	REGISTER_FN(env, isspace);
	REGISTER_FN(env, isalnum);
	REGISTER_FN(env, isalpha);
	REGISTER_FN(env, close);
	REGISTER_FN(env, inet_pton);
	REGISTER_FN(env, inet_ntop);
	REGISTER_FN(env, getaddrinfo);
	REGISTER_FN(env, freeaddrinfo);
	REGISTER_FN(env, sigaction);
	REGISTER_FN(env, alarm);
	REGISTER_FN(env, fcntl);
	REGISTER_FN(env, poll);
	REGISTER_FN(env, recv);
	REGISTER_FN(env, __errno);

	auto ctype_addr = this->_memory.get_next_word_addr();
	this->_memory.allocate(sizeof(emu__ctype_));
	this->_memory.copy(ctype_addr, &emu__ctype_, sizeof(emu__ctype_));
	env.program_loader().add_stub_symbol(ctype_addr, "_ctype_");

	auto tolower_tab_addr = this->_memory.get_next_word_addr();
	this->_memory.allocate(sizeof(emu__tolower_tab_));
	this->_memory.copy(tolower_tab_addr, &emu__tolower_tab_, sizeof(emu__tolower_tab_));
	env.program_loader().add_stub_symbol(tolower_tab_addr, "_tolower_tab_");

	auto toupper_tab_addr = this->_memory.get_next_word_addr();
	this->_memory.allocate(sizeof(emu__toupper_tab_));
	this->_memory.copy(toupper_tab_addr, &emu__toupper_tab_, sizeof(emu__toupper_tab_));
	env.program_loader().add_stub_symbol(toupper_tab_addr, "_toupper_tab_");

	auto errno_addr = this->_memory.get_next_word_addr();
	this->_memory.allocate(0x4);
	this->_memory.write_word(errno_addr, 0x0);
	_errno_addr = errno_addr;

	REGISTER_SYSCALL(env, fcntl, 0x37);
	REGISTER_SYSCALL(env, openat, 0x142);

	REGISTER_FN_RN(env, emu_glGetString, "glGetString");
	REGISTER_FN_RN(env, emu_glGetIntegerv, "glGetIntegerv");
	REGISTER_FN_RN(env, emu_glGetFloatv, "glGetFloatv");
	REGISTER_FN_RN(env, emu_glPixelStorei, "glPixelStorei");
	REGISTER_FN_RN(env, emu_glCreateShader, "glCreateShader");
	REGISTER_FN_RN(env, emu_glShaderSource, "glShaderSource");
	REGISTER_FN_RN(env, emu_glCompileShader, "glCompileShader");
	REGISTER_FN_RN(env, emu_glGetShaderiv, "glGetShaderiv");
	REGISTER_FN_RN(env, emu_glCreateProgram, "glCreateProgram");
	REGISTER_FN_RN(env, emu_glAttachShader, "glAttachShader");
	REGISTER_FN_RN(env, emu_glBindAttribLocation, "glBindAttribLocation");
	REGISTER_FN_RN(env, emu_glLinkProgram, "glLinkProgram");
	REGISTER_FN_RN(env, emu_glDeleteShader, "glDeleteShader");
	REGISTER_FN_RN(env, emu_glGetShaderSource, "glGetShaderSource");
	REGISTER_FN_RN(env, emu_glGetUniformLocation, "glGetUniformLocation");
	REGISTER_FN_RN(env, emu_glEnable, "glEnable");
	REGISTER_FN_RN(env, emu_glDisable, "glDisable");
	REGISTER_FN_RN(env, emu_glUniform1i, "glUniform1i");
	REGISTER_FN_RN(env, emu_glUniform4fv, "glUniform4fv");
	REGISTER_FN_RN(env, emu_glUniformMatrix4fv, "glUniformMatrix4fv");
	REGISTER_FN_RN(env, emu_glUseProgram, "glUseProgram");
	REGISTER_FN_RN(env, emu_glBindBuffer, "glBindBuffer");
	REGISTER_FN_RN(env, emu_glBindTexture, "glBindTexture");
	REGISTER_FN_RN(env, emu_glActiveTexture, "glActiveTexture");
	REGISTER_FN_RN(env, emu_glBufferData, "glBufferData");
	REGISTER_FN_RN(env, emu_glTexParameteri, "glTexParameteri");
	REGISTER_FN_RN(env, emu_glGenTextures, "glGenTextures");
	REGISTER_FN_RN(env, emu_glDeleteTextures, "glDeleteTextures");
	REGISTER_FN_RN(env, emu_glGenBuffers, "glGenBuffers");
	REGISTER_FN_RN(env, emu_glDeleteBuffers, "glDeleteBuffers");
	REGISTER_FN_RN(env, emu_glBlendFunc, "glBlendFunc");
	REGISTER_FN_RN(env, emu_glClearDepthf, "glClearDepthf");
	REGISTER_FN_RN(env, emu_glDepthFunc, "glDepthFunc");
	REGISTER_FN_RN(env, emu_glClearColor, "glClearColor");
	REGISTER_FN_RN(env, emu_glViewport, "glViewport");
	REGISTER_FN_RN(env, emu_glTexImage2D, "glTexImage2D");
	REGISTER_FN_RN(env, emu_glDrawArrays, "glDrawArrays");
	REGISTER_FN_RN(env, emu_glDrawElements, "glDrawElements");
	REGISTER_FN_RN(env, emu_glClear, "glClear");
	REGISTER_FN_RN(env, emu_glVertexAttribPointer, "glVertexAttribPointer");
	REGISTER_FN_RN(env, emu_glEnableVertexAttribArray, "glEnableVertexAttribArray");
	REGISTER_FN_RN(env, emu_glDisableVertexAttribArray, "glDisableVertexAttribArray");
	REGISTER_FN_RN(env, emu_glBufferSubData, "glBufferSubData");
	REGISTER_FN_RN(env, emu_glLineWidth, "glLineWidth");
	REGISTER_FN_RN(env, emu_glGetError, "glGetError");
	REGISTER_FN_RN(env, emu_glScissor, "glScissor");
	REGISTER_FN_RN(env, emu_glGetShaderInfoLog, "glGetShaderInfoLog");
	REGISTER_FN_RN(env, emu_glUniform1f, "glUniform1f");
	REGISTER_FN_RN(env, emu_glUniform2f, "glUniform2f");
	REGISTER_FN_RN(env, emu_glUniform3f, "glUniform3f");
	REGISTER_FN_RN(env, emu_glBindRenderbuffer, "glBindRenderbuffer");
	REGISTER_FN_RN(env, emu_glBindFramebuffer, "glBindFramebuffer");
	REGISTER_FN_RN(env, emu_glFramebufferTexture2D, "glFramebufferTexture2D");
	REGISTER_FN_RN(env, emu_glGenRenderbuffers, "glGenRenderbuffers");
	REGISTER_FN_RN(env, emu_glFramebufferRenderbuffer, "glFramebufferRenderbuffer");
	REGISTER_FN_RN(env, emu_glGenFramebuffers, "glGenFramebuffers");
	REGISTER_FN_RN(env, emu_glCheckFramebufferStatus, "glCheckFramebufferStatus");
	REGISTER_FN_RN(env, emu_glUniform4f, "glUniform4f");
}
