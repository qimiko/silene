#include "pthread.h"

#include <atomic>

// hi!
// https://github.com/aosp-mirror/platform_bionic/blob/main/libc/bionic/pthread_mutex.cpp

// on android, mutexes are split between normal mutexes and pi mutexes
// i don't think there's any pi mutexes in gd, so i'm not bothering to implement it yet
namespace {
struct BionicMutex {
	std::atomic_uint16_t state;
	std::atomic_uint16_t owner_tid;
};

constexpr std::uint16_t MUTEXATTR_SHARED_MASK = 0x0010;
constexpr std::uint16_t MUTEXATTR_TYPE_MASK = 0x000f;

constexpr std::uint16_t field_to_bits(std::uint16_t val, std::uint16_t shift, std::uint16_t bits) {
	return (val & ((1 << bits) - 1)) << shift;
}

constexpr std::uint16_t mutex_type_to_bits(std::uint16_t t) {
	return field_to_bits(t, 14, 2);
}

constexpr std::uint16_t field_mask(std::uint16_t shift, std::uint16_t bits) {
	return ((1 << bits) - 1) << shift;
}

constexpr std::uint16_t MUTEX_STATE_MASK = field_mask(0, 2);
constexpr std::uint16_t MUTEX_COUNTER_MASK = field_mask(2, 11);
constexpr std::uint16_t MUTEX_SHARED_MASK = field_mask(13, 1);
constexpr std::uint16_t MUTEX_TYPE_MASK = field_mask(14, 2);

constexpr std::uint16_t MUTEX_TYPE_BITS_NORMAL = mutex_type_to_bits(0);
constexpr std::uint16_t MUTEX_TYPE_BITS_RECURSIVE = mutex_type_to_bits(1);
constexpr std::uint16_t MUTEX_TYPE_BITS_ERRORCHECK = mutex_type_to_bits(2);

constexpr std::uint16_t mutex_state_to_bits(std::uint16_t v) {
	return field_to_bits(v, 0, 2);
}

constexpr bool is_mutex_destroyed(std::uint16_t mutex_state) {
    return mutex_state == 0xffff;
}

constexpr std::uint16_t MUTEX_STATE_BITS_UNLOCKED = mutex_state_to_bits(0);
constexpr std::uint16_t MUTEX_STATE_BITS_LOCKED_UNCONTENDED = mutex_state_to_bits(1);
constexpr std::uint16_t MUTEX_STATE_BITS_LOCKED_CONTENDED = mutex_state_to_bits(2);

constexpr bool mutex_state_bits_is_unlocked(std::uint16_t v) {
	return (v & MUTEX_STATE_MASK) == MUTEX_STATE_BITS_UNLOCKED;
}

constexpr bool mutex_state_bits_is_locked_uncontended(std::uint16_t v) {
	return (v & MUTEX_STATE_MASK) == MUTEX_STATE_BITS_LOCKED_UNCONTENDED;
}

constexpr bool mutex_state_bits_is_locked_contended(std::uint16_t v) {
	return (v & MUTEX_STATE_MASK) == MUTEX_STATE_BITS_LOCKED_CONTENDED;
}

constexpr std::uint16_t mutex_state_bits_flip_contention(std::uint16_t v) {
	return v ^ (MUTEX_STATE_BITS_LOCKED_CONTENDED ^ MUTEX_STATE_BITS_LOCKED_UNCONTENDED);
}

constexpr std::uint16_t MUTEX_COUNTER_BITS_ONE = field_to_bits(1, 2, 11);

constexpr bool mutex_counter_bits_will_overflow(std::uint16_t v) {
	return (v & MUTEX_COUNTER_MASK) == MUTEX_COUNTER_MASK;
}

constexpr bool mutex_counter_bits_is_zero(std::uint16_t v) {
	return (v & MUTEX_COUNTER_MASK) == 0;
}

std::int32_t mutex_try_lock(BionicMutex* mutex, std::uint16_t shared) {
	std::uint16_t unlocked = shared | MUTEX_STATE_BITS_UNLOCKED;
	std::uint16_t locked_uncontended = shared | MUTEX_STATE_BITS_LOCKED_UNCONTENDED;

	auto old_state = unlocked;
	if (std::atomic_compare_exchange_strong_explicit(&mutex->state, &old_state, locked_uncontended, std::memory_order_acquire, std::memory_order_relaxed)) {
		return 0;
	}

	return 16; // EBUSY
}

std::int32_t mutex_lock(BionicMutex* mutex, std::uint16_t shared) {
	if (mutex_try_lock(mutex, shared) == 0) {
		return 0;
	}

	std::uint16_t unlocked = shared | MUTEX_STATE_BITS_UNLOCKED;
	std::uint16_t locked_contended = shared | MUTEX_STATE_BITS_LOCKED_CONTENDED;

	while (std::atomic_exchange_explicit(&mutex->state, locked_contended, std::memory_order_acquire) != unlocked) {
		// this is probably the most concerning part of a futex swap
		std::atomic_wait(&mutex->state, locked_contended);
	}

	return 0;
}

std::int32_t mutex_wait(BionicMutex* mutex, std::uint16_t shared, std::uint16_t old_state) {
	// i lied. this is probably the most conerning futex swap
	std::atomic_wait(&mutex->state, old_state);
	return 0;
}

std::int32_t mutex_recursive_increment(BionicMutex* mutex, std::uint16_t old_state) {
	if (mutex_counter_bits_will_overflow(old_state)) {
		return 11; // EAGAIN
	}

	std::atomic_fetch_add_explicit(&mutex->state, MUTEX_COUNTER_BITS_ONE, std::memory_order_relaxed);
	return 0;
}

std::int32_t mutex_lock_with_timeout(Environment& env, BionicMutex* mutex) {
	auto old_state = std::atomic_load_explicit(&mutex->state, std::memory_order_relaxed);
	auto mtype = old_state & MUTEX_TYPE_MASK;
	auto shared = old_state & MUTEX_SHARED_MASK;

	if (mtype == MUTEX_TYPE_BITS_NORMAL) {
		return mutex_lock(mutex, shared);
	}

	// why does 1.0 have a recursive mutex
	auto tid = env.thread_id();
	if (tid == std::atomic_load_explicit(&mutex->owner_tid, std::memory_order_relaxed)) {
		if (mtype == MUTEX_TYPE_BITS_ERRORCHECK) {
			return 45; // EDEADLK
		}
		return mutex_recursive_increment(mutex, old_state);
	}

	auto unlocked = mtype | shared | MUTEX_STATE_BITS_UNLOCKED;
	auto locked_uncontended = mtype | shared | MUTEX_STATE_BITS_LOCKED_UNCONTENDED;
	auto locked_contended = mtype | shared | MUTEX_STATE_BITS_LOCKED_CONTENDED;

	if (old_state == unlocked) {
		if (std::atomic_compare_exchange_strong_explicit(&mutex->state, &old_state, locked_uncontended, std::memory_order_acquire, std::memory_order_relaxed)) {
			std::atomic_store_explicit(&mutex->owner_tid, tid, std::memory_order_relaxed);
			return 0;
		}
	}

	while (true) {
		if (old_state == unlocked) {
			if (std::atomic_compare_exchange_weak_explicit(&mutex->state, &old_state, locked_contended, std::memory_order_acquire, std::memory_order_relaxed)) {
				std::atomic_store_explicit(&mutex->owner_tid, tid, std::memory_order_relaxed);
				return 0;
			}
			continue;
		} else if (mutex_state_bits_is_locked_uncontended(old_state)) {
			auto new_state = mutex_state_bits_flip_contention(old_state);
			if (!std::atomic_compare_exchange_weak_explicit(&mutex->state, &old_state, new_state, std::memory_order_relaxed, std::memory_order_relaxed)) {
				continue;
			}

			old_state = new_state;
		}

		mutex_wait(mutex, shared, old_state);
		old_state = std::atomic_load_explicit(&mutex->state, std::memory_order_relaxed);
	}
}

void mutex_unlock(BionicMutex* mutex, std::uint16_t shared) {
	auto unlocked = shared | MUTEX_STATE_BITS_UNLOCKED;
	auto locked_contended = shared | MUTEX_STATE_BITS_LOCKED_CONTENDED;

	// the bionic source has a million comments for this
	if (std::atomic_exchange_explicit(&mutex->state, unlocked, std::memory_order_release) == locked_contended) {
		std::atomic_notify_one(&mutex->state);
	}
}

}

std::int32_t emu_pthread_mutex_init(Environment& env, std::uint32_t mutex_ptr, std::uint32_t attr_ptr) {
	spdlog::info("pthread_mutex_init({:#x})", mutex_ptr);

	auto mutex_obj = env.memory_manager().read_bytes<BionicMutex>(mutex_ptr);
	std::memset(mutex_obj, 0, sizeof(BionicMutex));

	if (attr_ptr == 0) {
		std::atomic_store(&mutex_obj->state, MUTEX_TYPE_BITS_NORMAL);
		return 0;
	}

	std::uint16_t state = 0;
	auto attr = env.memory_manager().read_word(attr_ptr);

	if ((attr & MUTEXATTR_SHARED_MASK) != 0) {
		state |= MUTEX_SHARED_MASK;
	}

	switch (attr & MUTEXATTR_TYPE_MASK) {
		case 0:
			state |= MUTEX_TYPE_BITS_NORMAL;
			break;
		case 1:
			state |= MUTEX_TYPE_BITS_RECURSIVE;
			break;
		case 2:
			state |= MUTEX_TYPE_BITS_ERRORCHECK;
			break;
		default:
			return 22; // EINVAL
	}

	// i'm not handling pimutexes unless i have to

	std::atomic_store(&mutex_obj->state, state);
	std::atomic_store(&mutex_obj->owner_tid, 0);

	return 0;
}

std::uint32_t emu_pthread_mutex_lock(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("pthread_mutex_lock({:#x})", mutex_ptr);

	if (mutex_ptr == 0) {
		return 22; // EINVAL
	}

	auto mutex_obj = env.memory_manager().read_bytes<BionicMutex>(mutex_ptr);
	auto old_state = std::atomic_load_explicit(&mutex_obj->state, std::memory_order_relaxed);
	auto mtype = old_state & MUTEX_TYPE_MASK;

	if (mtype == MUTEX_TYPE_BITS_NORMAL) {
		auto shared = old_state & MUTEX_SHARED_MASK;
		if (mutex_try_lock(mutex_obj, shared) == 0) {
			return 0;
		}
	}

	if (is_mutex_destroyed(old_state)) {
		throw std::runtime_error("mutex_lock on already destroyed mutex");
	}

	return mutex_lock_with_timeout(env, mutex_obj);
}

std::uint32_t emu_pthread_mutex_unlock(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("pthread_mutex_unlock({:#x})", mutex_ptr);

	if (mutex_ptr == 0) {
		return 22; // EINVAL
	}

	auto mutex_obj = env.memory_manager().read_bytes<BionicMutex>(mutex_ptr);
	auto old_state = std::atomic_load_explicit(&mutex_obj->state, std::memory_order_relaxed);
	auto mtype = old_state & MUTEX_TYPE_MASK;
	auto shared = old_state & MUTEX_SHARED_MASK;

	if (mtype == MUTEX_TYPE_BITS_NORMAL) {
		mutex_unlock(mutex_obj, shared);
		return 0;
	}

	if (is_mutex_destroyed(old_state)) {
		throw std::runtime_error("unlocking destroyed mutex");
	}

	auto tid = env.thread_id();
	if (tid != std::atomic_load_explicit(&mutex_obj->owner_tid, std::memory_order_relaxed)) {
		return 1; // EPERM
	}

	if (!mutex_counter_bits_is_zero(old_state)) {
		std::atomic_fetch_sub_explicit(&mutex_obj->state, MUTEX_COUNTER_BITS_ONE, std::memory_order_relaxed);
		return 0;
	}

	std::atomic_store_explicit(&mutex_obj->owner_tid, 0, std::memory_order_relaxed);
	auto unlocked = mtype | shared | MUTEX_STATE_BITS_UNLOCKED;
	old_state = std::atomic_exchange_explicit(&mutex_obj->state, unlocked, std::memory_order_release);
	if (mutex_state_bits_is_locked_contended(old_state)) {
		std::atomic_notify_one(&mutex_obj->state);
	}

	return 0;
}

std::int32_t emu_pthread_mutex_destroy(Environment& env, std::uint32_t mutex_ptr) {
	spdlog::info("pthread_mutex_destroy({:#x})", mutex_ptr);

	auto mutex_obj = env.memory_manager().read_bytes<BionicMutex>(mutex_ptr);
	auto old_state = std::atomic_load_explicit(&mutex_obj->state, std::memory_order_relaxed);

	if (is_mutex_destroyed(old_state)) {
		throw std::runtime_error("destroy called on already destroyed mutex");
	}

	if (mutex_state_bits_is_unlocked(old_state)
			&& std::atomic_compare_exchange_strong_explicit(
				&mutex_obj->state, &old_state, 0xffff, std::memory_order_relaxed, std::memory_order_relaxed
			)) {
		return 0;
	}

	return 16; // EBUSY
}
