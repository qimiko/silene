#include "stdio.h"

#include <sstream>

template <typename T>
std::string perform_printf(Environment& env, const std::string& format_str, T& v) {
	std::stringstream ss;

	auto str_data = format_str.data();
	auto in_format = false;

	// world's greatest printf implementation
	// gd doesn't require a very complicated one, tbh

	while (*str_data) {
		auto c = *str_data;
		switch (c) {
			case '%':
				if (in_format) {
					ss << '%';
					in_format = false;
				} else {
					in_format = true;
				}
				break;
			case 'i':
				if (in_format) {
					ss << v.template next<std::int32_t>();
					in_format = false;
				} else {
					ss << c;
				}
				break;
			case 'f':
				if (in_format) {
					ss << v.template next<float>();
					in_format = false;
				} else {
					ss << c;
				}
				break;
			default:
				ss << c;
				break;
		}

		str_data++;
	}

	return ss.str();
}

std::uint32_t emu_fopen(Environment& env, std::uint32_t filename_ptr, std::uint32_t mode_ptr) {
	auto filename = env.memory_manager()->read_bytes<char>(filename_ptr);
	auto mode = env.memory_manager()->read_bytes<char>(mode_ptr);

	return env.libc().open_file(filename, mode);
}

std::int32_t emu_fclose(Environment& env, std::uint32_t file) {
	return env.libc().close_file(file);
}

std::uint32_t emu_fwrite(Environment& env, std::uint32_t buffer_ptr, std::uint32_t size, std::uint32_t count, std::uint32_t stream_ptr) {
	spdlog::info("TODO: fwrite");

	return 0;
}

std::int32_t emu_fseek(Environment& env, std::uint32_t file_ref, std::int32_t offset, std::int32_t origin) {
	return env.libc().seek_file(file_ref, offset, origin);
}

std::int32_t emu_ftell(Environment& env, std::uint32_t file) {
	return env.libc().tell_file(file);
}

std::int32_t emu_fread(Environment& env, std::uint32_t buf_ptr, std::uint32_t size, std::uint32_t count, std::uint32_t file_ref) {
	auto buf = env.memory_manager()->read_bytes<void>(buf_ptr);
	return env.libc().read_file(buf, size, count, file_ref);
}

std::int32_t emu_fputs(Environment& env, std::uint32_t str_ptr, std::uint32_t stream_ptr) {
	spdlog::info("TODO: fputs");
	return -1;
}

std::int32_t emu_sprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr) {
	// auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	spdlog::info("TODO: sprintf - {}", format);
	return 0;
}

std::int32_t emu_vsprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr) {
	// auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	spdlog::info("TODO: vsprintf - {}", format);
	return 0;
}

std::int32_t emu_vsnprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t output_size, std::uint32_t format_ptr, VaList va) {
	auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, va);
	std::strncpy(output, formatted.c_str(), output_size);

	return 0;
}

std::int32_t emu_fprintf(Environment& env, std::uint32_t output_file, std::uint32_t format_ptr) {
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	spdlog::info("TODO: fprintf({}) - {}", output_file, format);
	return 0;
}

std::int32_t emu_fputc(Environment& env, std::int32_t character, std::uint32_t output_file) {
	spdlog::info("TODO: fputc({}, {:#x})", output_file, character);

	return character;
}
