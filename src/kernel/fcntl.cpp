#include "kernel.h"

std::int32_t kernel_openat(Environment& env, std::int32_t dirfd, std::uint32_t pathname_ptr, std::int32_t flags, std::int32_t mode) {
	auto filename = env.memory_manager()->read_bytes<char>(pathname_ptr);

	spdlog::info("TODO: openat({}, {}, {}, {})", dirfd, filename, flags, mode);
	return 0;
}