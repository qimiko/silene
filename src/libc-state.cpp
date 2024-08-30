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

	return next;
}

void LibcState::free_memory(std::uint32_t vaddr) {
}

std::uint32_t LibcState::reallocate_memory(std::uint32_t vaddr, std::uint32_t size) {
	return vaddr;
}

void LibcState::register_destructor(StaticDestructor destructor) {
	this->_destructors.push_back(destructor);
}

void LibcState::pre_init(Environment& env) {
	REGISTER_FN(env, sin);
	REGISTER_FN(env, sinf);
	REGISTER_FN(env, __cxa_atexit);
	REGISTER_FN(env, __cxa_finalize);
	REGISTER_FN(env, __gnu_Unwind_Find_exidx);
	REGISTER_FN(env, strcmp);
	REGISTER_FN(env, strncmp);
	REGISTER_FN(env, btowc);
	REGISTER_FN(env, wctype);
	REGISTER_FN(env, wctob);
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
	REGISTER_FN(env, abort);
	REGISTER_FN(env, strlen);
	REGISTER_FN(env, memset);
	REGISTER_FN(env, setlocale);
	REGISTER_FN(env, fopen);
	REGISTER_FN(env, fwrite);
	REGISTER_FN(env, fputs);
	REGISTER_FN(env, getsockopt);
	REGISTER_FN(env, calloc);
	REGISTER_FN(env, __stack_chk_fail);
	REGISTER_FN(env, memcmp);
	REGISTER_FN(env, __android_log_print);
	REGISTER_FN(env, sprintf);
	REGISTER_FN(env, strcpy);
	REGISTER_FN(env, strncpy);
	REGISTER_FN(env, vsprintf);
	REGISTER_FN(env, vsnprintf);
	REGISTER_FN(env, setjmp);
	REGISTER_FN(env, longjmp);
	REGISTER_FN(env, fprintf);
	REGISTER_FN(env, fputc);
	REGISTER_FN(env, strtol);
	REGISTER_FN(env, strtod);
}
