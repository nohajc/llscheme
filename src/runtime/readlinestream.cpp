#include <cstring>
#include <algorithm>
#include "../../include/runtime/readlinestream.hpp"
#include "../../include/linenoise/linenoise.h"
#include "../../include/debug.hpp"

readlinebuf::readlinebuf(size_t pb): put_back_max(pb), buffer(max(pb, size_t(1))) {
    char * end = &buffer.front() + buffer.size();
    setg(end, end, end);
    prompt = "";
}

streambuf::int_type readlinebuf::underflow() {
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }

    int base = 0;
    int start = 0;

    if (eback() == &buffer[base]) {
        size_t diff = egptr() - &buffer[base];
        size_t put_back = diff < put_back_max ? diff : put_back_max;

        memmove(&buffer[base], egptr() - put_back, put_back);
        buffer.resize(put_back);
        start += put_back;
    }
    else {
        buffer.resize(0);
    }

    char * line = linenoise(prompt.c_str());
    prompt = "";
    if (!line) {
        return traits_type::eof();
    }
    linenoiseHistoryAdd(line);
    size_t n = strlen(line);

    buffer.insert(buffer.begin() + start, line, line + n);
    buffer.push_back('\n');
    n++;
    free(line);

    setg(&buffer[base], &buffer[start], &buffer[start] + n);

    return traits_type::to_int_type(*gptr());
}

