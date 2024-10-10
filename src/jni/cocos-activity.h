#pragma once

#ifndef _JNI_COCOS_ACTIVITY_H
#define _JNI_COCOS_ACTIVITY_H

#include "../environment.h"

void jni_show_message_box(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id, VaList args) {
	auto title_obj = args.next<std::uint32_t>();
	auto message_obj = args.next<std::uint32_t>();

	auto title = std::get<std::string>(env.jni().get_ref_value(title_obj));
	auto message = std::get<std::string>(env.jni().get_ref_value(message_obj));

	spdlog::warn("Application called showMessageBox! [{}] {}", title, message);
}

#endif