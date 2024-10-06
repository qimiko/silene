#include "stdio.h"

#include "../libc-state.h"

#include <sstream>
#include <charconv>
#include <unordered_set>
#include <iomanip>

template <typename T>
std::string perform_printf(Environment& env, const std::string& format_str, T& v) {
	std::stringstream ss;

	auto str_data = format_str.data();
	auto in_format = false;

	// world's greatest printf implementation
	// gd doesn't require a very complicated one, tbh

	std::string format_flags{};
	std::unordered_set valid_flags{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	while (*str_data) {
		auto c = *str_data;
		switch (c) {
			case '%':
				if (in_format) {
					ss << '%';
					in_format = false;
				} else {
					format_flags.clear();
					in_format = true;
				}
				break;
			case 's':
				if (in_format) {
					auto ptr = v.template next<std::uint32_t>();
					auto substr = env.memory_manager()->read_bytes<char>(ptr);
					ss << substr;

					in_format = false;
				} else {
					ss << c;
				}
				break;
			case 'i':
			case 'd':
				if (in_format) {
					auto zero_pad = false;
					auto padding_size = 0;

					if (!format_flags.empty()) {
						zero_pad = format_flags[0] == '0' ? true : false;

						if (format_flags.size() > 1) {
							format_flags = format_flags.substr(1, format_flags.size() - 1);
							padding_size = std::atoi(format_flags.c_str());
						}
					}

					auto current_flags = ss.flags();

					if (zero_pad) {
						ss << std::setfill('0');
					} else {
						ss << std::setfill(' ');
					}

					if (padding_size > 0) {
						ss << std::setw(padding_size);
					}

					ss << v.template next<std::int32_t>();
					in_format = false;

					ss.flags(current_flags);
				} else {
					ss << c;
				}
				break;
			case 'f':
			case 'g':
				// TODO: they are slightly different in presentation,
				// but not sure how to represent that with stringstreams
				if (in_format) {
					ss << v.template next<double>();
					in_format = false;
				} else {
					ss << c;
				}
				break;
			default:
				if (in_format) {
					if (valid_flags.contains(c)) {
						format_flags.push_back(c);
					} else {
						spdlog::info("encountered invalid flag specifier {}", c);
					}

					break;
				}

				ss << c;
				break;
		}

		str_data++;
	}

	spdlog::trace("performed printf: {} -> {}", format_str, ss.str());

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

std::int32_t emu_sprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr, Variadic v) {
	auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, v);
	std::strncpy(output, formatted.c_str(), formatted.size() + 1);

	return formatted.size();
}

std::int32_t emu_snprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t output_size, std::uint32_t format_ptr, Variadic v) {
	auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, v);
	std::strncpy(output, formatted.c_str(), output_size);

	return std::min(static_cast<std::uint32_t>(formatted.size()), output_size);
}

std::int32_t emu_vsprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t format_ptr, VaList va) {
	auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, va);
	std::strncpy(output, formatted.c_str(), formatted.size() + 1);

	return formatted.size();
}

std::int32_t emu_vsnprintf(Environment& env, std::uint32_t output_ptr, std::uint32_t output_size, std::uint32_t format_ptr, VaList va) {
	auto output = env.memory_manager()->read_bytes<char>(output_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, va);
	std::strncpy(output, formatted.c_str(), output_size);

	return std::min(static_cast<std::uint32_t>(formatted.size()), output_size);
}

std::int32_t emu_fprintf(Environment& env, std::uint32_t output_file, std::uint32_t format_ptr, Variadic v) {
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	auto formatted = perform_printf(env, format, v);

	spdlog::info("TODO: fprintf({}) -> {}", output_file, formatted);
	return 0;
}

std::int32_t emu_fputc(Environment& env, std::int32_t character, std::uint32_t output_file) {
	spdlog::info("TODO: fputc({}, {:#x})", output_file, character);

	return character;
}

template <typename T>
std::int32_t perform_sscanf(Environment& env, const std::string& src, const std::string& format, T& v) {
	spdlog::trace("sccanf(`{}`, `{}`)", src, format);

	auto fmt_data = format.data();
	auto src_data = src.data();

	auto in_specifier = false;
	auto fmt_specifier = '\0';

	std::unordered_set<char> valid_flags{'h'};
	std::string fmt_flags{};

	auto args_count = 0;

	auto inc_src = [&](char c) {
		if (c != *src_data) {
			// skip to end
			src_data = src_data + src.size();
			return;
		}

		src_data++;
		return;
	};

	while (*fmt_data && *src_data) {
		auto c = *fmt_data;

		switch (c) {
			case 'f':
			case 'd':
			case 'i':
			case 'u':
				if (in_specifier) {
					in_specifier = false;
					fmt_specifier = c;
					break;
				}
				inc_src(c);
				break;
			case '%':
				if (!in_specifier) {
					in_specifier = true;
					fmt_flags.clear();
					break;
				}
				inc_src(c);
				break;
			default:
				if (in_specifier) {
					if (valid_flags.contains(c)) {
						fmt_flags.push_back(c);
						break;
					} else {
						spdlog::info("unknown sscanf specifier {}", c);
					}
				}
				inc_src(c);
		}

		fmt_data++;

		if (fmt_specifier != '\0') {
			// skip whitespace in src
			while (*src_data && std::isspace(static_cast<unsigned char>(*src_data))) {
				src_data++;
			}

			if (!*src_data) {
				continue;
			}

			// dispatch parsing
			auto next_ptr = v.template next<std::uint32_t>();
			auto success = false;

			switch (fmt_specifier) {
				case 'f': {
					char* end_ptr = nullptr;
					auto ret = std::strtof(src_data, &end_ptr);

					spdlog::trace("parse float arg {} : *{:#x} -> {}", args_count, next_ptr, ret);

					if (end_ptr != src_data) {
						success = true;
						env.memory_manager()->write_word(next_ptr, std::bit_cast<std::uint32_t>(ret));
						src_data = end_ptr;
					}

					break;
				}
				case 'i': {
					char* end_ptr = nullptr;
					auto ret = std::strtol(src_data, &end_ptr, 0);

					auto write_size = 4;
					if (!fmt_flags.empty()) {
						if (fmt_flags == "hh") {
							write_size = 1;
						} else if (fmt_flags == "h") {
							write_size = 2;
						} else if (fmt_flags == "l") {
							write_size = 8;
						}
					}

					spdlog::trace("parse int arg {} : *{:#x} -> {}", args_count, next_ptr, ret);

					if (end_ptr != src_data) {
						success = true;
						switch (write_size) {
							case 1: {
								auto r_byte = static_cast<std::int8_t>(ret);
								env.memory_manager()->write_byte(next_ptr, std::bit_cast<std::uint8_t>(r_byte));
								break;
							}
							case 2: {
								auto r_hw = static_cast<std::int16_t>(ret);
								env.memory_manager()->write_halfword(next_ptr, std::bit_cast<std::uint16_t>(r_hw));
								break;
							}
							default:
							case 4: {
								auto r_w = static_cast<std::int32_t>(ret);
								env.memory_manager()->write_word(next_ptr, std::bit_cast<std::uint32_t>(r_w));
								break;
							}
							case 8: {
								env.memory_manager()->write_doubleword(next_ptr, std::bit_cast<std::uint64_t>(ret));
								break;
							}
						}
						src_data = end_ptr;
					}

					break;
				}
				case 'd': {
					char* end_ptr = nullptr;
					auto ret = static_cast<std::int32_t>(std::strtol(src_data, &end_ptr, 10));

					spdlog::trace("parse decimal arg {} : *{:#x} -> {}", args_count, next_ptr, ret);

					if (end_ptr != src_data) {
						success = true;
						env.memory_manager()->write_word(next_ptr, std::bit_cast<std::uint32_t>(ret));
						src_data = end_ptr;
					}

					break;
				}
				case 'u': {
					char* end_ptr = nullptr;
					auto ret = std::strtoul(src_data, &end_ptr, 10);

					spdlog::trace("parse unsigned decimal arg {} : *{:#x} -> {}", args_count, next_ptr, ret);

					if (end_ptr != src_data) {
						success = true;
						env.memory_manager()->write_word(next_ptr, static_cast<std::uint32_t>(ret));
						src_data = end_ptr;
					}

					break;
				}
				default:
					break;
			}

			if (success) {
				args_count += 1;
				fmt_specifier = '\0';
			} else {
				src_data = src_data + src.size();
				continue;
			}
		}
	}

	return args_count;
}

std::int32_t emu_sscanf(Environment& env, std::uint32_t buf_ptr, std::uint32_t format_ptr, Variadic v) {
	auto buf = env.memory_manager()->read_bytes<char>(buf_ptr);
	auto format = env.memory_manager()->read_bytes<char>(format_ptr);

	return perform_sscanf(env, buf, format, v);
}
