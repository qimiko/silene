#pragma once

#ifndef _LIBC_SEMAPHORE_H
#define _LIBC_SEMAPHORE_H

#include "../environment.h"

#include <atomic>

// much like pthread, this is based off the bionic source code
// https://github.com/aosp-mirror/platform_bionic/blob/main/libc/bionic/semaphore.cpp

namespace {

constexpr std::uint32_t SEM_VALUE_MAX = 0x3fffffff;
constexpr std::uint32_t SEMCOUNT_VALUE_SHIFT = 1;
constexpr std::uint32_t SEMCOUNT_SHARED_MASK = 0x00000001;
constexpr std::uint32_t SEMCOUNT_VALUE_MASK = 0xfffffffe;

constexpr std::uint32_t semcount_increment(std::uint32_t sval) {
	return (sval + (1U << SEMCOUNT_VALUE_SHIFT)) & SEMCOUNT_VALUE_MASK;
}

constexpr std::uint32_t semcount_decrement(std::uint32_t sval) {
	return (sval - (1U << SEMCOUNT_VALUE_SHIFT)) & SEMCOUNT_VALUE_MASK;
}

constexpr std::int32_t semcount_to_value(std::uint32_t sval) {
	return (static_cast<int>(sval) >> SEMCOUNT_VALUE_SHIFT);
}

constexpr std::uint32_t semcount_from_value(std::uint32_t val) {
	return ((val) << SEMCOUNT_VALUE_SHIFT) & SEMCOUNT_VALUE_MASK;
}

constexpr std::int32_t SEMCOUNT_ONE = semcount_from_value(1);
constexpr std::uint32_t SEMCOUNT_MINUS_ONE = semcount_from_value(~0U);

constexpr std::uint32_t sem_get_shared(std::atomic_uint32_t* sem_count_ptr) {
	return std::atomic_load_explicit(sem_count_ptr, std::memory_order_relaxed) & SEMCOUNT_SHARED_MASK;
}

constexpr std::int32_t sem_dec(std::atomic_uint32_t* sem_count_ptr) {
	auto old_value = std::atomic_load_explicit(sem_count_ptr, std::memory_order_relaxed);
	auto shared = old_value & SEMCOUNT_SHARED_MASK;

	do {
		if (semcount_to_value(old_value) < 0) {
			break;
		}
	} while (!std::atomic_compare_exchange_weak(sem_count_ptr, &old_value, semcount_decrement(old_value) | shared));

	return semcount_to_value(old_value);
}

constexpr std::int32_t sem_inc(std::atomic_uint32_t* sem_count_ptr) {
	auto old_value = std::atomic_load_explicit(sem_count_ptr, std::memory_order_relaxed);
	auto shared = old_value & SEMCOUNT_SHARED_MASK;
	auto new_value = 0u;

	do {
		if (semcount_to_value(old_value) == SEM_VALUE_MAX) {
			break;
		}

		if (semcount_to_value(old_value) < 0) {
			new_value = SEMCOUNT_ONE | shared;
		} else {
			new_value = semcount_increment(old_value) | shared;
		}
	} while (!std::atomic_compare_exchange_weak(sem_count_ptr, &old_value, new_value));

	return semcount_to_value(old_value);
}

}

std::uint32_t emu_sem_init(Environment& env, std::uint32_t sem_ptr, std::int32_t p_shared, std::uint32_t value) {
	// semaphores are pointers to atomic_uints on armv7
	// in this case we don't actually need to care about the layout, but this is probably easier...
	if (value > SEM_VALUE_MAX) {
		return -1;
	}

	auto count = semcount_from_value(value);
	if (p_shared != 0) {
		count |= SEMCOUNT_SHARED_MASK;
	}

	auto sem_atomic = env.memory_manager().read_bytes<std::atomic_uint32_t>(sem_ptr);

	// atomic_init is deprecated so i hope i'm doing the correct thing here
	std::atomic_store(sem_atomic, count);

	return 0;
}

std::int32_t emu_sem_post(Environment& env, std::uint32_t sem_ptr) {
	auto sem_atomic = env.memory_manager().read_bytes<std::atomic_uint32_t>(sem_ptr);

	auto old_value = sem_inc(sem_atomic);
	if (old_value < 0) {
		std::atomic_notify_all(sem_atomic);
	} else if (old_value == SEM_VALUE_MAX) {
		return -1;
	}

	return 0;
}

std::int32_t emu_sem_wait(Environment& env, std::uint32_t sem_ptr) {
	auto sem_atomic = env.memory_manager().read_bytes<std::atomic_uint32_t>(sem_ptr);
	auto shared = sem_get_shared(sem_atomic);

	while (true) {
		if (sem_dec(sem_atomic) > 0) {
			return 0;
		}

		std::atomic_wait(sem_atomic, shared | SEMCOUNT_MINUS_ONE);
	}

	return 0;
}

std::int32_t emu_sem_destroy(Environment& env, std::uint32_t sem_ptr) {
	// this too is unimplemented on bionic
	return 0;
}

#endif