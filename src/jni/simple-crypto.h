#pragma once

#ifndef _JNI_SIMPLE_CRYPTO_H
#define _JNI_SIMPLE_CRYPTO_H

#include "../environment.h"

std::uint32_t jni_decrypt_file_to_string(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id, VaList args) {
	auto file_ptr = args.next<std::uint32_t>();
	auto title = std::get<std::string>(env.jni().get_ref_value(file_ptr));

	spdlog::info("TODO: decrypt file {}", title);

	return env.jni().create_string_ref("");
}

#endif