#pragma once

#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

#include <cstdint>

#include "../environment.h"

std::int32_t kernel_openat(Environment& env, std::int32_t dirfd, std::uint32_t pathname, std::int32_t flags, std::int32_t mode);

std::int32_t kernel_fcntl(Environment& env, std::int32_t fd, std::int32_t op, std::uint32_t arg);
std::int32_t emu_fcntl(Environment& env, std::int32_t fd, std::int32_t op, std::uint32_t arg);

std::int32_t kernel_open(Environment& env, std::uint32_t filename_ptr, std::int32_t flags, std::int32_t mode);
std::int32_t emu_open(Environment& env, std::uint32_t filename_ptr, std::int32_t flags, std::int32_t mode);

std::int32_t kernel_fstat(Environment& env, std::int32_t fd, std::uint32_t buf_ptr);
std::int32_t emu_fstat(Environment& env, std::int32_t fd, std::uint32_t buf_ptr);

#endif