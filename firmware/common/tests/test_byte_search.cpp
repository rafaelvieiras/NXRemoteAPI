#include "catch.hpp"
#include "../include/ByteSearch.h"
#include <cstring>

using namespace ByteSearch;

TEST_CASE("Contains", "[byte_search]") {
    SECTION("Needle present in the middle") {
        const unsigned char buf[] = "xx_nxremoteapiyy";
        REQUIRE(Contains(buf, sizeof(buf) - 1, "_nxremoteapi", 12));
    }

    SECTION("Needle absent") {
        const unsigned char buf[] = "no match here";
        REQUIRE_FALSE(Contains(buf, sizeof(buf) - 1, "_nxremoteapi", 12));
    }

    SECTION("Needle would match just past haystack_len - must not read there") {
        // "_nxremoteapi" starts at index 2 but the declared length only covers half of
        // it - this is exactly the non-NUL-terminated recvfrom() scenario from issue #1:
        // a naive strstr() over a longer underlying buffer would find it anyway.
        const unsigned char buf[] = "xx_nxremoteapi";
        REQUIRE_FALSE(Contains(buf, 8, "_nxremoteapi", 12));
    }

    SECTION("Needle exactly fills haystack_len") {
        const unsigned char buf[] = "_nxremoteapi";
        REQUIRE(Contains(buf, 12, "_nxremoteapi", 12));
    }

    SECTION("Haystack shorter than needle") {
        const unsigned char buf[] = "short";
        REQUIRE_FALSE(Contains(buf, 5, "_nxremoteapi", 12));
    }

    SECTION("Empty needle always matches") {
        const unsigned char buf[] = "anything";
        REQUIRE(Contains(buf, sizeof(buf) - 1, "", 0));
    }

    SECTION("Empty haystack only matches empty needle") {
        REQUIRE_FALSE(Contains(nullptr, 0, "x", 1));
        REQUIRE(Contains(nullptr, 0, "", 0));
    }

    SECTION("Binary data (embedded zero bytes) doesn't confuse the search") {
        const unsigned char buf[] = {0x00, 0x84, 0x00, '_', 'n', 'x', 0x00, 'x'};
        REQUIRE(Contains(buf, sizeof(buf), "_nx", 3));
        REQUIRE_FALSE(Contains(buf, sizeof(buf), "_nxremoteapi", 12));
    }
}
