#include "libc-state.h"

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

#include "kernel/kernel.h"

#include "gl/gl-wrap.h"

std::uint32_t LibcState::allocate_memory(std::uint32_t size, bool zero_mem) {
	// TODO: this implementation is terrible.

	auto next = this->_memory->get_next_addr();

	// dword alignment
	if (next % 8 != 0) {
		auto next_aligned = ((next / 8) + 1) * 8;
		this->_memory->allocate(next_aligned - next);
		next = next_aligned;
	}

	this->_memory->allocate(size);

	if (zero_mem) {
		this->_memory->set(next, 0, size);
	}

	return next;
}

void LibcState::free_memory(std::uint32_t vaddr) {
}

std::uint32_t LibcState::reallocate_memory(std::uint32_t vaddr, std::uint32_t size) {
	auto new_ptr = this->allocate_memory(size, false);

	auto src = this->_memory->read_bytes<std::uint8_t>(vaddr);
	auto dest = this->_memory->read_bytes<std::uint8_t>(new_ptr);

	// we should really track memory sizes, this could create issues
	std::memcpy(dest, src, size);

	this->free_memory(vaddr);

	return new_ptr;
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
	this->_memory->write_word(addr, addr);

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

void LibcState::pre_init(Environment& env) {
	REGISTER_FN(env, sin);
	REGISTER_FN(env, sinf);
	REGISTER_FN(env, cos);
	REGISTER_FN(env, cosf);
	REGISTER_FN(env, sqrtf);
	REGISTER_FN(env, ceilf);
	REGISTER_FN(env, ceil);
	REGISTER_FN(env, round);
	REGISTER_FN(env, roundf);
	REGISTER_FN(env, atan2);
	REGISTER_FN(env, atan2f);
	REGISTER_FN(env, acos);
	REGISTER_FN(env, floor);
	REGISTER_FN(env, floorf);
	REGISTER_FN(env, powf);
	REGISTER_FN(env, pow);
	REGISTER_FN(env, fmodf);
	REGISTER_FN(env, fmod);
	REGISTER_FN(env, asinf);
	REGISTER_FN(env, tanf);
	REGISTER_FN(env, __cxa_atexit);
	REGISTER_FN(env, __cxa_finalize);
	REGISTER_FN(env, __gnu_Unwind_Find_exidx);
	REGISTER_FN(env, strcmp);
	REGISTER_FN(env, strncmp);
	REGISTER_FN(env, btowc);
	REGISTER_FN(env, wctype);
	REGISTER_FN(env, wctob);
	REGISTER_FN(env, atoi);
	REGISTER_FN(env, pthread_key_create);
	REGISTER_FN(env, pthread_once);
	REGISTER_FN(env, pthread_mutex_lock);
	REGISTER_FN(env, pthread_mutex_unlock);
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
	REGISTER_FN(env, __stack_chk_fail);
	REGISTER_FN(env, memcmp);
	REGISTER_FN(env, __android_log_print);
	REGISTER_FN(env, sprintf);
	REGISTER_FN(env, strcpy);
	REGISTER_FN(env, strncpy);
	REGISTER_FN(env, vsprintf);
	REGISTER_FN(env, vsnprintf);
	REGISTER_FN(env, sscanf);
	REGISTER_FN(env, setjmp);
	REGISTER_FN(env, longjmp);
	REGISTER_FN(env, fprintf);
	REGISTER_FN(env, fputc);
	REGISTER_FN(env, strtol);
	REGISTER_FN(env, strtod);
	REGISTER_FN(env, strstr);
	REGISTER_FN(env, strtok);
	REGISTER_FN(env, gettimeofday);
	REGISTER_FN(env, time);
	REGISTER_FN(env, getenv);
	REGISTER_FN(env, lrand48);
	REGISTER_FN(env, arc4random);
	REGISTER_FN(env, ftime);
	REGISTER_FN(env, srand48);

	REGISTER_SYSCALL(env, openat, 0x142);

	REGISTER_FN(env, glGetString);
	REGISTER_FN(env, glGetIntegerv);
	REGISTER_FN(env, glGetFloatv);
	REGISTER_FN(env, glPixelStorei);
	REGISTER_FN(env, glCreateShader);
	REGISTER_FN(env, glShaderSource);
	REGISTER_FN(env, glCompileShader);
	REGISTER_FN(env, glGetShaderiv);
	REGISTER_FN(env, glCreateProgram);
	REGISTER_FN(env, glAttachShader);
	REGISTER_FN(env, glBindAttribLocation);
	REGISTER_FN(env, glLinkProgram);
	REGISTER_FN(env, glDeleteShader);
	REGISTER_FN(env, glGetShaderSource);
	REGISTER_FN(env, glGetUniformLocation);
	REGISTER_FN(env, glEnable);
	REGISTER_FN(env, glDisable);
	REGISTER_FN(env, glUniform1i);
	REGISTER_FN(env, glUniform4fv);
	REGISTER_FN(env, glUniformMatrix4fv);
	REGISTER_FN(env, glUseProgram);
	REGISTER_FN(env, glBindBuffer);
	REGISTER_FN(env, glBindTexture);
	REGISTER_FN(env, glActiveTexture);
	REGISTER_FN(env, glBufferData);
	REGISTER_FN(env, glTexParameteri);
	REGISTER_FN(env, glGenTextures);
	REGISTER_FN(env, glGenBuffers);
	REGISTER_FN(env, glDeleteBuffers);
	REGISTER_FN(env, glBlendFunc);
	REGISTER_FN(env, glClearDepthf);
	REGISTER_FN(env, glDepthFunc);
	REGISTER_FN(env, glClearColor);
	REGISTER_FN(env, glViewport);
	REGISTER_FN(env, glTexImage2D);
	REGISTER_FN(env, glDrawArrays);
	REGISTER_FN(env, glDrawElements);
	REGISTER_FN(env, glClear);
	REGISTER_FN(env, glVertexAttribPointer);
	REGISTER_FN(env, glEnableVertexAttribArray);
	REGISTER_FN(env, glDisableVertexAttribArray);
	REGISTER_FN(env, glBufferSubData);
	REGISTER_FN(env, glLineWidth);
	REGISTER_FN(env, glGetError);
	REGISTER_FN(env, glScissor);
}
