#pragma once

#ifndef _LIBC_SEMAPHORE_H
#define _LIBC_SEMAPHORE_H

#include "../environment.h"

std::uint32_t emu_sem_init(Environment& env, std::uint32_t sem_ptr, std::int32_t p_shared, std::uint32_t value);
std::int32_t emu_sem_post(Environment& env, std::uint32_t sem_ptr);
std::int32_t emu_sem_wait(Environment& env, std::uint32_t sem_ptr);
std::int32_t emu_sem_destroy(Environment& env, std::uint32_t sem_ptr);

#endif