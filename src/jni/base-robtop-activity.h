#pragma once

#ifndef _JNI_BASE_ROBTOP_ACTIVITY_H
#define _JNI_BASE_ROBTOP_ACTIVITY_H

#include "../environment.h"

std::uint32_t jni_get_user_id(Environment& env) {
	auto& jni = env.jni();

	// i just generated a random uuid. there's no networking support, so i don't care yet
	auto uuid = "cc52b577-524d-41b1-b3f0-e46d13dd1452";

	return jni.create_string_ref(uuid);
}

#endif