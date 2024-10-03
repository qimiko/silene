#pragma once

#ifndef _LIBC_PTHREAD_H
#define _LIBC_PTHREAD_H

#include "../environment.h"

std::uint32_t emu_pthread_key_create(Environment& env, std::uint32_t key_ptr, std::uint32_t destructor_fn_ptr) {
	spdlog::info("TODO: pthread_key_create");
	// this should be implemented, one day. not sure how honestly
	return 88; // ENOSYS
}

std::uint32_t emu_pthread_once(Environment& env, std::uint32_t once_control_ptr, std::uint32_t init_fn_ptr) {
	spdlog::info("TODO: pthread_once");
	// call init_fn if not already called. shouldn't be hard to implement
	return 88; // ENOSYS
}

std::uint32_t emu_pthread_mutex_lock(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("TODO: pthread_mutex_lock");
	// no mutexes yet, no need to lock
	return 0;
}

std::uint32_t emu_pthread_mutex_unlock(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("TODO: pthread_mutex_unlock");
	return 0;
}

std::int32_t emu_pthread_cond_broadcast(Environment& env, std::uint32_t cond_ptr) {
	spdlog::info("TODO: pthread_cond_broadcast");
	return 0;
}

#endif