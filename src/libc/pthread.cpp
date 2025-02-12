#include "pthread.h"

#include <atomic>
#include <type_traits>

#include "../android-application.hpp"

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
	// on android, the pthread_once_t is an atomic_int. that's convenient, but only if you assume that all atomic_ints are the same
	// anyways, the other thing is that this means we can implement our pthread_once in the same way
	// source: https://github.com/aosp-mirror/platform_bionic/blob/main/libc/bionic/pthread_once.cpp
	static_assert(sizeof(std::atomic_int) == 4, "atomic_int != int != 4 bytes");

	spdlog::trace("pthread_once({:#x}, {:#x})", once_control_ptr, init_fn_ptr);

	auto once_control_obj = env.memory_manager().read_bytes<std::atomic_int>(once_control_ptr);

	auto old_value = std::atomic_load_explicit(once_control_obj, std::memory_order_acquire);
	while (true) {
		if (old_value == 2) {
			return 0;
		}

		if (!std::atomic_compare_exchange_weak_explicit(once_control_obj, &old_value, 1, std::memory_order_acquire, std::memory_order_acquire)) {
			continue;
		}

		if (old_value == 0) {
			env.run_func(init_fn_ptr);

			std::atomic_store_explicit(once_control_obj, 2, std::memory_order_release);

			std::atomic_notify_all(once_control_obj);
			return 0;
		}

		std::atomic_wait_explicit(once_control_obj, old_value, std::memory_order_acquire);
		old_value = std::atomic_load_explicit(once_control_obj, std::memory_order_acquire);
	}
}

std::int32_t emu_pthread_create(Environment& env, std::uint32_t thread_ptr, std::uint32_t attr_ptr, std::uint32_t start_routine, std::uint32_t arg) {
	if (attr_ptr != 0) {
		spdlog::info("TODO: pthread_create with non-zero attr");
	}

	auto tid = env.application().create_thread(start_routine, arg);
	env.memory_manager().write_word(thread_ptr, tid);

	return 0;
}

std::int32_t emu_pthread_detach(Environment& env, std::uint32_t thread) {
	env.application().detach_thread(thread);

	return 0;
}

std::int32_t emu_pthread_exit(Environment& env, std::uint32_t ret_val_ptr) {
	spdlog::info("TODO: pthread_exit");
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

// directly ripped from bionic, again
// https://android.googlesource.com/platform/bionic/+/master/libc/bionic/pthread_cond.cpp
// pthread_cond_t is a pointer to an atomic_uint32_t. i think this was exported publicly at some point?

namespace {
constexpr std::uint8_t BIONIC_COND_SHARED_MASK = 0x1;
constexpr std::uint8_t BIONIC_COND_CLOCK_MASK = 0x2;
constexpr std::uint8_t BIONIC_COND_FLAGS_MASK = BIONIC_COND_SHARED_MASK | BIONIC_COND_CLOCK_MASK;

constexpr std::uint8_t BIONIC_COND_COUNTER_STEP = 0x4;

std::int32_t __pthread_cond_pulse(Environment& env, std::uint32_t cond_ptr, std::int32_t thread_count) {
	auto cond = env.memory_manager().read_bytes<std::atomic_uint32_t>(cond_ptr);
  std::atomic_fetch_add_explicit(cond, BIONIC_COND_COUNTER_STEP, std::memory_order_relaxed);
	std::atomic_notify_all(cond);
  return 0;
}

std::int32_t __pthread_cond_timedwait(Environment& env, std::uint32_t cond_ptr, std::uint32_t mutex_ptr) {
	auto cond = env.memory_manager().read_bytes<std::atomic_uint32_t>(cond_ptr);

  auto old_state = std::atomic_load_explicit(cond, std::memory_order_relaxed);

  emu_pthread_mutex_unlock(env, mutex_ptr);

	std::atomic_wait(cond, old_state);

  emu_pthread_mutex_lock(env, mutex_ptr);

  return 0;
}

}

std::int32_t emu_pthread_cond_init(Environment& env, std::uint32_t cond_ptr, std::uint32_t attr_ptr) {
	spdlog::info("pthread_cond_init({:#x})", cond_ptr);

	if (cond_ptr == 0) {
		return 22; // EINVAL
	}

	auto cond = env.memory_manager().read_bytes<std::atomic_uint32_t>(cond_ptr);

	auto init_state = 0u;
	if (attr_ptr != 0) {
		init_state = env.memory_manager().read_word(attr_ptr) & BIONIC_COND_FLAGS_MASK;
	}
	std::atomic_store_explicit(cond, init_state, std::memory_order_relaxed);

	return 0;
}

std::int32_t emu_pthread_cond_destroy(Environment& env, std::uint32_t cond_ptr) {
	spdlog::info("pthread_cond_destroy({:#x})", cond_ptr);

	auto cond = env.memory_manager().read_bytes<std::atomic_uint32_t>(cond_ptr);
	std::atomic_store_explicit(cond, 0xdeadc04d, std::memory_order_relaxed);

	return 0;
}

std::int32_t emu_pthread_cond_broadcast(Environment& env, std::uint32_t cond_ptr) {
	spdlog::info("pthread_cond_broadcast({:#x})", cond_ptr);

	return __pthread_cond_pulse(env, cond_ptr, std::numeric_limits<std::int32_t>::max());
}

std::int32_t emu_pthread_cond_signal(Environment& env, std::uint32_t cond_ptr) {
	spdlog::info("pthread_cond_signal({:#x})", cond_ptr);

  return __pthread_cond_pulse(env, cond_ptr, 1);
}

std::int32_t emu_pthread_cond_wait(Environment& env, std::uint32_t cond_ptr, std::uint32_t mutex_ptr) {
	spdlog::info("pthread_cond_wait({:#x}, {:#x})", cond_ptr, mutex_ptr);

	return __pthread_cond_timedwait(env, cond_ptr, mutex_ptr);
}
