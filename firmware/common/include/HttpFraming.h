#pragma once

// Pure byte/string logic pulled out of firmware/core/main.cpp's request-handling
// path so it can be unit-tested on the host without libnx or a real socket. No
// dependency on <switch.h>/MockLibnx.h at all - this is genuinely platform
// independent, unlike the rest of firmware/common/.
//
// Behavior here is deliberately a byte-for-byte match of what main.cpp used to do
// inline, quirks included (e.g. route matching is a substring search, not an
// anchored prefix match) - this is an extraction for testability, not a rewrite.
// If you fix a quirk, do it as its own change with its own test, not silently
// while moving code.
namespace HttpFraming {

// Mirrors the body-completeness check that used to live inline in
// handle_client() (firmware/core/main.cpp): true once the full request has
// arrived (per its own Content-Length, if any), false if more recv() calls are
// still needed. This is exactly the check that, when it was too permissive,
// caused the historical bug where a POST body split across two TCP segments
// looked like an empty body to handlers further down (see the comment above
// handle_client() in main.cpp).
//
// `buffer` must be null-terminated within `bytes_read` bytes (same contract the
// original inline code relied on). Returns false (nothing more to wait for) if
// the header block ("\r\n\r\n") hasn't fully arrived yet, or if there's no
// Content-Length header at all - matching the original's behavior of only
// chasing a body once it knows how big one is supposed to be.
bool HasCompleteBody(const char* buffer, int bytes_read);

// Matches core's actual route dispatch, which is `strstr(buffer, route)` - a
// substring search over the whole raw request (headers and body included), not
// an anchored match against just the request line. Extracted verbatim (not
// hardened) so behavior doesn't shift as a side effect of this refactor.
bool MatchesRoute(const char* buffer, const char* route);

// Mirrors the X-API-Token auth check in handle_client(): builds the exact header
// value ("X-API-Token: <token>") and reports whether `buffer` contains it
// (again, substring search over the whole raw request, same as the original).
bool HasValidApiToken(const char* buffer, const char* expected_token);

} // namespace HttpFraming
