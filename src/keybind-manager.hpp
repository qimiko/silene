#pragma once

#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <toml.hpp>

#include "android-environment.hpp"

#ifndef _KEYBIND_MANAGER_HPP
#define _KEYBIND_MANAGER_HPP

class KeybindManager {
private:
    toml::basic_value<toml::type_config> root;
    AndroidEnvironment& _env;
public:
    void handle(int key, int scancode, int action) {
        auto key_name = glfwGetKeyName(key, scancode);

        if (!key_name) return;

        auto pos = toml::find_or<std::vector<int>>(root, key_name, {});

        if (pos.size() != 2) return;

        auto jni_env_ptr = _env.jni().get_env_ptr();

        if (action == GLFW_PRESS) {
			_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin",
				jni_env_ptr, 0, 1, static_cast<float>(pos[0]), static_cast<float>(pos[1]));
		} else if (action == GLFW_RELEASE) {
			_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd",
				jni_env_ptr, 0, 1, static_cast<float>(pos[0]), static_cast<float>(pos[1]));
		}
    }

    KeybindManager(std::string path, AndroidEnvironment& env): root(toml::parse(path)), _env(env) {}
};

#endif