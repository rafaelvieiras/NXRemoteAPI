#pragma once

#include "json.hpp"

#include <string>

// Type-checked accessors for nlohmann::json, safe to call on ANY json value -
// including ones that aren't objects at all - without ever going through
// nlohmann's own throwing get<T>()/value() path. See issue #2: firmware/core
// compiles with -fno-exceptions (firmware/core/Makefile), and the vendored
// json.hpp maps its internal JSON_THROW(...) to std::abort() in that mode - so
// json::value()/get<T>() throwing type_error (wrong type for an existing key, or
// called on a value that isn't an object at all) kills the whole always-on core
// process instead of raising anything catchable. A single authenticated request
// with a subtly wrong JSON type (a number where a string was expected, or an
// array element that isn't an object) was enough to deterministically abort it.
//
// These fall back to `def` in exactly that situation, per the issue's own
// proposed fix - matching json::value()'s "missing key" behavior, but extended
// to also cover "key present with the wrong type" and "not an object at all".
namespace JsonSafeAccess {

// `def` unless `obj` is a JSON object, `key` exists in it, and the value at that
// key is actually a JSON string.
std::string GetString(const nlohmann::json& obj, const char* key, const std::string& def);

// `def` unless `obj` is a JSON object, `key` exists in it, and the value at that
// key is a JSON number (integer or floating-point, same coercion json::value<int>()
// itself would apply).
int GetInt(const nlohmann::json& obj, const char* key, int def);

} // namespace JsonSafeAccess
