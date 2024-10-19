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
