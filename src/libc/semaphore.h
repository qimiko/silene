#pragma once

#ifndef _LIBC_SEMAPHORE_H
#define _LIBC_SEMAPHORE_H

#include "../environment.h"

std::uint32_t emu_sem_init(Environment& env, std::uint32_t sem_ptr, std::int32_t p_shared, std::uint32_t value) {
	spdlog::info("TODO: sem_init");
	return 0;
}

std::int32_t emu_sem_post(Environment& env, std::uint32_t sem_ptr) {
	spdlog::info("TODO: sem_post");
	return 0;
}

#endif