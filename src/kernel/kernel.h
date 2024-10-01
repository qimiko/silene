#pragma once

#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

#include <cstdint>

#include "../environment.h"

std::int32_t kernel_openat(Environment& env, std::int32_t dirfd, std::uint32_t pathname, std::int32_t flags, std::int32_t mode);

#endif