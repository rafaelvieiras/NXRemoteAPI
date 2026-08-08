#include "HttpFraming.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace HttpFraming {

bool HasCompleteBody(const char* buffer, int bytes_read) {
    const char* body_start = strstr(buffer, "\r\n\r\n");
    if (!body_start) return false;

    const char* cl_hdr = strstr(buffer, "Content-Length:");
    if (!cl_hdr) cl_hdr = strstr(buffer, "content-length:");
    if (!cl_hdr) return true; // no body expected, nothing to wait for

    long content_length = strtol(cl_hdr + 15, nullptr, 10);
    long have_body = bytes_read - static_cast<long>((body_start + 4) - buffer);
    return have_body >= content_length;
}

bool MatchesRoute(const char* buffer, const char* route) {
    return strstr(buffer, route) != nullptr;
}

bool HasValidApiToken(const char* buffer, const char* expected_token) {
    char token_header[128];
    snprintf(token_header, sizeof(token_header), "X-API-Token: %s", expected_token);
    return strstr(buffer, token_header) != nullptr;
}

} // namespace HttpFraming
