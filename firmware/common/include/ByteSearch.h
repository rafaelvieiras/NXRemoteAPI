#pragma once

#include <cstddef>

// Pure byte-buffer logic pulled out of firmware/core/main.cpp's mdns_responder()
// so it can be unit-tested on the host, same rationale as HttpFraming.h. No
// dependency on <switch.h>/MockLibnx.h - platform independent.
namespace ByteSearch {

// Length-bounded substring search over raw, possibly non-NUL-terminated bytes
// (e.g. a UDP datagram straight out of recvfrom()). Equivalent to memmem(), which
// this project avoids relying on directly - it's gated behind __GNU_VISIBLE on
// devkitA64's libc, not worth poking _GNU_SOURCE feature-test macros for one call
// site. Never reads past `haystack + haystack_len`, unlike strstr() on a buffer
// that isn't guaranteed to contain a NUL - see issue #1.
bool Contains(const unsigned char* haystack, size_t haystack_len,
              const char* needle, size_t needle_len);

} // namespace ByteSearch
