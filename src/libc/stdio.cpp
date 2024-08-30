#include "stdio.h"

std::uint32_t emu_fopen(Environment& env, std::uint32_t filename_ptr, std::uint32_t mode_ptr) {
	auto filename = env.memory_manager()->read_bytes<char>(filename_ptr);
	spdlog::info("TODO: fopen({})", filename);

	return 0;
}

std::uint32_t emu_fwrite(Environment& env, std::uint32_t buffer_ptr, std::uint32_t size, std::uint32_t count, std::uint32_t stream_ptr) {
	spdlog::info("TODO: fwrite");

	return 0;
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

	spdlog::info("TODO: vsnprintf - {}", format);
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
