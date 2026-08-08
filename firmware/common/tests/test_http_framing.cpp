#include "catch.hpp"
#include "../include/HttpFraming.h"
#include <cstring>

using namespace HttpFraming;

TEST_CASE("HasCompleteBody", "[http_framing]") {
    SECTION("Headers not fully arrived yet") {
        const char* buf = "POST /command HTTP/1.1\r\nContent-Length: 10";
        REQUIRE_FALSE(HasCompleteBody(buf, (int)strlen(buf)));
    }

    SECTION("No Content-Length header - nothing to wait for") {
        const char* buf = "GET /info HTTP/1.1\r\nHost: switch\r\n\r\n";
        REQUIRE(HasCompleteBody(buf, (int)strlen(buf)));
    }

    SECTION("Body split across two TCP segments - the historical bug this guards against") {
        // curl sent a 10-byte body, but only 4 bytes have arrived so far (as if the
        // second send() hasn't landed yet) - this is exactly the scenario that used
        // to make POST /command misreport "Missing action" on a well-formed request.
        const char* buf = "POST /command HTTP/1.1\r\nContent-Length: 10\r\n\r\n{\"a\":";
        REQUIRE_FALSE(HasCompleteBody(buf, (int)strlen(buf)));
    }

    SECTION("Full body already present") {
        const char* buf = "POST /command HTTP/1.1\r\nContent-Length: 10\r\n\r\n{\"a\":\"bc\"}";
        REQUIRE(HasCompleteBody(buf, (int)strlen(buf)));
    }

    SECTION("More than the declared body has arrived (e.g. keep-alive garbage) - still complete") {
        const char* buf = "POST /command HTTP/1.1\r\nContent-Length: 4\r\n\r\n{\"a\"}extra-bytes";
        REQUIRE(HasCompleteBody(buf, (int)strlen(buf)));
    }

    SECTION("Lowercase content-length header is honored") {
        const char* buf = "POST /command HTTP/1.1\r\ncontent-length: 10\r\n\r\n{\"a\":";
        REQUIRE_FALSE(HasCompleteBody(buf, (int)strlen(buf)));
    }
}

TEST_CASE("MatchesRoute mirrors core's substring-based dispatch", "[http_framing]") {
    SECTION("Route at the start of the request line matches") {
        const char* buf = "GET /info HTTP/1.1\r\nHost: switch\r\n\r\n";
        REQUIRE(MatchesRoute(buf, "GET /info"));
    }

    SECTION("Absent route does not match") {
        const char* buf = "GET /info HTTP/1.1\r\n\r\n";
        REQUIRE_FALSE(MatchesRoute(buf, "GET /titles"));
    }

    SECTION("Known quirk: a route string appearing anywhere else in the request also matches") {
        // Documents current behavior (see HttpFraming.h) rather than endorsing it -
        // core's real dispatch has always been strstr()-based, not anchored to the
        // request line. A header/body that happens to contain another route's text
        // would also match here; that's a pre-existing property being preserved by
        // this extraction, not introduced by it.
        const char* buf = "GET /info HTTP/1.1\r\nX-Debug: mentions GET /titles in passing\r\n\r\n";
        REQUIRE(MatchesRoute(buf, "GET /titles"));
    }
}

TEST_CASE("HasValidApiToken", "[http_framing]") {
    SECTION("Correct token header present") {
        const char* buf = "GET /titles HTTP/1.1\r\nX-API-Token: secret123\r\n\r\n";
        REQUIRE(HasValidApiToken(buf, "secret123"));
    }

    SECTION("Wrong token does not match") {
        const char* buf = "GET /titles HTTP/1.1\r\nX-API-Token: wrong\r\n\r\n";
        REQUIRE_FALSE(HasValidApiToken(buf, "secret123"));
    }

    SECTION("Missing header entirely does not match") {
        const char* buf = "GET /titles HTTP/1.1\r\nHost: switch\r\n\r\n";
        REQUIRE_FALSE(HasValidApiToken(buf, "secret123"));
    }
}
