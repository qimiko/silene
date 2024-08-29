#pragma once

#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H

#include "../environment.h"
#include "../syscall-translator.hpp"

std::uint32_t emu_fopen(Environment& env, std::uint32_t filename_ptr, std::uint32_t mode_ptr);
std::uint32_t emu_fwrite(Environment& env, std::uint32_t buffer_ptr, std::uint32_t size, std::uint32_t count, std::uint32_t stream_ptr);
std::int32_t emu_fputs(Environment& env, std::uint32_t str_ptr, std::uint32_t stream_ptr);
std::int32_t emu_sprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr);
std::int32_t emu_vsprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr);
std::int32_t emu_vsnprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t output_size, std::uint32_t format_ptr, VaList va);
std::int32_t emu_fprintf(Environment& env, std::uint32_t output_file, std::uint32_t format_ptr);
std::int32_t emu_fputc(Environment& env, std::int32_t character, std::uint32_t output_file);

#endif