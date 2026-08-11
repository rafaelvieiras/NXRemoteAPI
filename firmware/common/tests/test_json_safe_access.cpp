#include "catch.hpp"
#include "../include/JsonSafeAccess.h"
#include "../include/json.hpp"

using json = nlohmann::json;

TEST_CASE("GetString", "[json_safe_access]") {
    SECTION("Key present with correct type") {
        json j = json::parse(R"({"action": "reboot"})");
        REQUIRE(JsonSafeAccess::GetString(j, "action", "") == "reboot");
    }

    SECTION("Key missing falls back to default") {
        json j = json::parse(R"({"other": "x"})");
        REQUIRE(JsonSafeAccess::GetString(j, "action", "fallback") == "fallback");
    }

    SECTION("Key present with wrong type (number) falls back instead of aborting") {
        // This is the exact issue #2 scenario: POST /command {"action": 123} used to
        // abort the whole process via nlohmann's throwing get<T>() under -fno-exceptions.
        json j = json::parse(R"({"action": 123})");
        REQUIRE(JsonSafeAccess::GetString(j, "action", "fallback") == "fallback");
    }

    SECTION("Value is not an object at all") {
        // The POST /button {"sequence": ["A", "B"]} scenario: each `step` here is a
        // plain JSON string, not an object - .value() on it aborts unconditionally,
        // regardless of which key is asked for.
        json step = json::parse(R"("A")");
        REQUIRE(JsonSafeAccess::GetString(step, "button", "fallback") == "fallback");
    }

    SECTION("Value is a JSON array, not an object") {
        json j = json::parse(R"([1, 2, 3])");
        REQUIRE(JsonSafeAccess::GetString(j, "action", "fallback") == "fallback");
    }
}

TEST_CASE("GetInt", "[json_safe_access]") {
    SECTION("Key present with correct type") {
        json j = json::parse(R"({"duration_ms": 250})");
        REQUIRE(JsonSafeAccess::GetInt(j, "duration_ms", 100) == 250);
    }

    SECTION("Key missing falls back to default") {
        json j = json::parse(R"({"other": 1})");
        REQUIRE(JsonSafeAccess::GetInt(j, "duration_ms", 100) == 100);
    }

    SECTION("Key present with wrong type (string) falls back instead of aborting") {
        // Exact issue #2 scenario: {"duration_ms": "100"} used to abort via get<int>().
        json j = json::parse(R"({"duration_ms": "100"})");
        REQUIRE(JsonSafeAccess::GetInt(j, "duration_ms", 100) == 100);
    }

    SECTION("Value is not an object at all") {
        json step = json::parse("42");
        REQUIRE(JsonSafeAccess::GetInt(step, "duration_ms", 100) == 100);
    }
}
