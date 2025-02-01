#pragma once

#include <string>
#include <vector>
#include <optional>
#include <utility>

#include <string_view>
#include <toml.hpp>

#include "android-environment.hpp"


#ifndef _KEYBIND_MANAGER_HPP
#define _KEYBIND_MANAGER_HPP

class KeybindManager {
private:
    toml::basic_value<toml::type_config> root;
public:
    std::optional<std::pair<float, float>> handle(std::string_view key_name) {
        if (key_name.empty()) return std::nullopt;

        auto pos = toml::find_or<std::vector<int>>(root, key_name, {});

        if (pos.size() != 2) return std::nullopt;

        return std::make_optional<std::pair<float, float>>(
            static_cast<float>(pos[0]),
            static_cast<float>(pos[1])
        );
    }

    KeybindManager(std::string path): root(toml::parse(path)) {}
};

#endif
