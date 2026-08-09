#include "ByteSearch.h"

#include <cstring>

namespace ByteSearch {

bool Contains(const unsigned char* haystack, size_t haystack_len,
              const char* needle, size_t needle_len) {
    if (needle_len == 0) return true;
    if (haystack_len < needle_len) return false;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        if (memcmp(haystack + i, needle, needle_len) == 0) return true;
    }
    return false;
}

} // namespace ByteSearch
