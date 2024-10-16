#pragma once

#ifndef _LIBC_PTHREAD_H
#define _LIBC_PTHREAD_H

#include "../environment.h"

#include <atomic>
#include <type_traits>

std::int32_t emu_pthread_key_create(Environment& env, std::uint32_t key_ptr, std::uint32_t destructor_fn_ptr) {
	spdlog::info("TODO: pthread_key_create");
	// this should be implemented, one day. not sure how honestly
	return 88; // ENOSYS
}

std::int32_t emu_pthread_key_delete(Environment& env, std::uint32_t key_ptr) {
	spdlog::info("TODO: pthread_key_delete");
	return 0;
}

std::uint32_t emu_pthread_once(Environment& env, std::uint32_t once_control_ptr, std::uint32_t init_fn_ptr) {
	spdlog::info("TODO: pthread_once");
	return 0;
}

std::int32_t emu_pthread_create(Environment& env, std::uint32_t thread_ptr, std::uint32_t attr_ptr, std::uint32_t start_routine, std::uint32_t arg) {
	spdlog::info("TODO: pthread_create");
	return 0;
}

std::int32_t emu_pthread_detach(Environment& env, std::uint32_t thread) {
	spdlog::info("TODO: pthread_detach");
	return 0;
}

std::int32_t emu_pthread_mutex_init(Environment& env, std::uint32_t mutex_ptr, std::uint32_t attr_ptr) {
	spdlog::info("TODO: pthread_mutex_init");
	return 0;
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

std::int32_t emu_pthread_mutex_destroy(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("TODO: pthread_mutex_destroy");
	return 0;
}

std::int32_t emu_pthread_cond_broadcast(Environment& env, std::uint32_t cond_ptr) {
	spdlog::info("TODO: pthread_cond_broadcast");
	return 0;
}

std::int32_t emu_pthread_cond_wait(Environment& env, std::uint32_t cond_ptr, std::uint32_t mutex_ptr) {
	spdlog::info("TODO: pthread_cond_wait");
	return 0;
}

std::uint32_t emu_pthread_getspecific(Environment& env, std::int32_t key) {
	spdlog::info("TODO: pthread_getspecific");
	return 0;
}

std::int32_t emu_pthread_setspecific(Environment& env, std::int32_t key, std::uint32_t value) {
	spdlog::info("TODO: pthread_setspecific");
	return 0;
}

std::int32_t emu_pthread_exit(Environment& env, std::uint32_t ret_val_ptr) {
	spdlog::info("TODO: pthread_exit");
	return 0;
}

#endif