#include "JsonSafeAccess.h"

namespace JsonSafeAccess {

std::string GetString(const nlohmann::json& obj, const char* key, const std::string& def) {
    if (!obj.is_object()) return def;
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) return def;
    return it->get<std::string>();
}

int GetInt(const nlohmann::json& obj, const char* key, int def) {
    if (!obj.is_object()) return def;
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_number()) return def;
    return it->get<int>();
}

} // namespace JsonSafeAccess
