#include "kernel.h"

std::int32_t kernel_openat(Environment& env, std::int32_t dirfd, std::uint32_t pathname_ptr, std::int32_t flags, std::int32_t mode) {
	auto filename = env.memory_manager().read_bytes<char>(pathname_ptr);

	spdlog::info("TODO: openat({}, {}, {}, {})", dirfd, filename, flags, mode);
	return 0;
}

std::int32_t kernel_fcntl(Environment& env, std::int32_t fd, std::int32_t op, std::uint32_t arg) {
	if (op != F_GETFL && op != F_SETFL) {
		// not sure about the mapping for these, so warn if anything unexpected comes
		spdlog::info("TODO: fcntl({}, {}, {})", fd, op, arg);
	}

	return fcntl(fd, op, arg);
}

std::int32_t emu_fcntl(Environment& env, std::int32_t fd, std::int32_t op, std::uint32_t arg) {
	return kernel_fcntl(env, fd, op, arg);
}

std::int32_t kernel_open(Environment& env, std::uint32_t filename_ptr, std::int32_t flags, std::int32_t mode) {
	auto filename = env.memory_manager().read_bytes<char>(filename_ptr);

	spdlog::info("open({}, {:#x}, {:#x})", filename, flags, mode);

	return open(filename, flags, mode);
}

std::int32_t emu_open(Environment& env, std::uint32_t filename_ptr, std::int32_t flags, std::int32_t mode) {
	return kernel_open(env, filename_ptr, flags, mode);
}

std::int32_t kernel_fstat(Environment& env, std::int32_t fd, std::uint32_t buf_ptr) {
	spdlog::info("TODO: fstat({}, {:#x})", fd, buf_ptr);

	return -1;
}

std::int32_t emu_fstat(Environment& env, std::int32_t fd, std::uint32_t buf_ptr) {
	return kernel_fstat(env, fd, buf_ptr);
}
