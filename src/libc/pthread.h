#pragma once

#ifndef _LIBC_PTHREAD_H
#define _LIBC_PTHREAD_H

#include "../environment.h"

std::int32_t emu_pthread_key_create(Environment& env, std::uint32_t key_ptr, std::uint32_t destructor_fn_ptr);
std::int32_t emu_pthread_key_delete(Environment& env, std::uint32_t key_ptr);
std::uint32_t emu_pthread_once(Environment& env, std::uint32_t once_control_ptr, std::uint32_t init_fn_ptr);
std::int32_t emu_pthread_create(Environment& env, std::uint32_t thread_ptr, std::uint32_t attr_ptr, std::uint32_t start_routine, std::uint32_t arg);
std::int32_t emu_pthread_detach(Environment& env, std::uint32_t thread);
std::int32_t emu_pthread_mutex_init(Environment& env, std::uint32_t mutex_ptr, std::uint32_t attr_ptr);
std::uint32_t emu_pthread_mutex_lock(Environment& env, std::uint32_t mutex_ptr);
std::uint32_t emu_pthread_mutex_unlock(Environment& env, std::uint32_t mutex_ptr);
std::int32_t emu_pthread_mutex_destroy(Environment& env, std::uint32_t mutex_ptr);
std::int32_t emu_pthread_cond_init(Environment& env, std::uint32_t cond_ptr, std::uint32_t attr_ptr);
std::int32_t emu_pthread_cond_broadcast(Environment& env, std::uint32_t cond_ptr);
std::int32_t emu_pthread_cond_signal(Environment& env, std::uint32_t cond_ptr);
std::int32_t emu_pthread_cond_destroy(Environment& env, std::uint32_t cond_ptr);
std::int32_t emu_pthread_cond_wait(Environment& env, std::uint32_t cond_ptr, std::uint32_t mutex_ptr);
std::uint32_t emu_pthread_getspecific(Environment& env, std::int32_t key);
std::int32_t emu_pthread_setspecific(Environment& env, std::int32_t key, std::uint32_t value);
std::int32_t emu_pthread_exit(Environment& env, std::uint32_t ret_val_ptr);

#endif