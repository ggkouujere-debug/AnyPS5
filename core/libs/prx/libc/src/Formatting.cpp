#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <algorithm>
#include <cstdarg>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <climits>
#include <cerrno>
#include <cstdlib>
#include <memory>

#include "prx/libc/include/General.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include "prx/libc/include/WindowsFormatting.hpp"

#ifdef _WIN32
#include "prx/libc/include/WindowsScanning.hpp"
#endif

#if defined(__GLIBC__)
extern "C" {
int __isoc99_vsscanf(const char*, const char*, std::va_list) noexcept;
int __isoc99_vfscanf(std::FILE*, const char*, std::va_list);
}
#endif

namespace {

#ifndef _WIN32
bool HasGuestWideFormat(const char* format) {
    if (format == nullptr) return false;
    while ((format = std::strchr(format, '%')) != nullptr) {
        ++format;
        if (*format == '%') { ++format; continue; }
        while (*format && std::strchr("-+ #0'0123456789.*$", *format)) ++format;
        if (*format == 'S' || *format == 'C' || *format == 'm' || (*format == 'l' && (format[1] == 's' || format[1] == 'c'))) return true;
        if (*format) ++format;
    }
    return false;
}

bool HasWideScanFormat(const char* format) {
    if (format == nullptr) return false;
    while ((format = std::strchr(format, '%')) != nullptr) {
        ++format;
        if (*format == '%') { ++format; continue; }
        while (*format && std::strchr("-+ #0'0123456789.*$", *format)) ++format;
        if (*format == 'S' || *format == 'C' || (*format == 'l' && format[1] != '\0' && std::strchr("sc[", format[1])))
            return true;
        if (*format == '[') {
            ++format;
            if (*format == '^') ++format;
            if (*format == ']') ++format;
            while (*format && *format != ']') ++format;
        }
        if (*format) ++format;
    }
    return false;
}

bool DecodeUtf8(const char*& p, char32_t& code) {
    const unsigned char c = static_cast<unsigned char>(*p);
    if (c < 0x80) {
        if (c == 0) return false;
        code = c;
        ++p;
        return true;
    }
    unsigned length;
    char32_t value;
    if ((c & 0xe0) == 0xc0) { length = 2; value = c & 0x1f; }
    else if ((c & 0xf0) == 0xe0) { length = 3; value = c & 0x0f; }
    else if ((c & 0xf8) == 0xf0) { length = 4; value = c & 0x07; }
    else return false;
    for (unsigned i = 1; i < length; ++i) {
        const unsigned char d = static_cast<unsigned char>(p[i]);
        if (d == 0 || (d & 0xc0) != 0x80) return false;
        value = (value << 6) | (d & 0x3f);
    }
    if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
        (length == 4 && value < 0x10000) || value > 0x10ffff ||
        (value >= 0xd800 && value <= 0xdfff)) return false;
    code = value;
    p += length;
    return true;
}

unsigned EncodeUtf16(char32_t code, char16_t* out) {
    if (code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) code = 0xfffd;
    if (code < 0x10000) {
        out[0] = static_cast<char16_t>(code);
        return 1;
    }
    code -= 0x10000;
    out[0] = static_cast<char16_t>(0xd800 + (code >> 10));
    out[1] = static_cast<char16_t>(0xdc00 + (code & 0x3ff));
    return 2;
}

void ParseWideScanset(const std::string& specifier, std::vector<char32_t>& set, bool& negated) {
    set.clear();
    negated = false;
    const char* sp = specifier.c_str();
    if (*sp != '[') return;
    ++sp;
    if (*sp == '^') { negated = true; ++sp; }
    if (*sp == ']') {
        set.push_back(static_cast<char32_t>(']'));
        ++sp;
    }
    while (*sp != ']' && *sp != '\0') {
        const char* q = sp;
        char32_t first;
        if (!DecodeUtf8(q, first)) break;
        if (*q == '-' && q[1] != ']' && q[1] != '\0') {
            const char* r = q + 1;
            char32_t last;
            if (DecodeUtf8(r, last) && first <= last) {
                for (char32_t c = first; c <= last; ++c) set.push_back(c);
                sp = r;
                continue;
            }
        }
        set.push_back(first);
        sp = q;
    }
}
#endif

std::string HostLength(const std::string& length, char conversion) {
#ifdef _WIN32
    if (length == "l" && std::strchr("diouxXn", conversion) != nullptr) return "ll";
#else
    (void)conversion;
#endif
    return length;
}

int HostVsscanf(const char* input, const char* format, std::va_list args) {
#if defined(__GLIBC__)
    return __isoc99_vsscanf(input, format, args);
#else
    return std::vsscanf(input, format, args);
#endif
}

int HostSscanf(const char* input, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HostVsscanf(input, format, args);
    va_end(args);
    return result;
}

#ifndef _WIN32
int HostVfscanf(std::FILE* stream, const char* format, std::va_list args) {
#if defined(__GLIBC__)
    return __isoc99_vfscanf(stream, format, args);
#else
    return std::vfscanf(stream, format, args);
#endif
}
#endif

template <typename NextPointer, typename NextCapacity>
int ScanGuest(const char* buffer, const char* format, bool secure, NextPointer nextPointer, NextCapacity nextCapacity) {
    if (buffer == nullptr || format == nullptr) return EOF;
    const char* input = buffer;
    int assigned = 0;
    bool converted = false;
    const char* cursor = format;
    const auto finish = [](int result) { return result; };
    while (*cursor != '\0') {
        if (std::isspace(static_cast<unsigned char>(*cursor))) {
            while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
            while (std::isspace(static_cast<unsigned char>(*input))) ++input;
            continue;
        }
        if (*cursor != '%' || cursor[1] == '%') {
            const char expected = *cursor == '%' ? '%' : *cursor;
            cursor += *cursor == '%' ? 2 : 1;
            if (expected == '%') while (std::isspace(static_cast<unsigned char>(*input))) ++input;
            if (*input != expected) return finish(!converted && *input == '\0' ? EOF : assigned);
            ++input;
            continue;
        }
        ++cursor;
        const bool suppress = *cursor == '*';
        if (suppress) ++cursor;
        unsigned long width = 0;
        while (std::isdigit(static_cast<unsigned char>(*cursor))) width = width * 10 + static_cast<unsigned long>(*cursor++ - '0');
        const char* lengthStart = cursor;
        while (std::strchr("hljztL", *cursor) != nullptr && *cursor != '\0') ++cursor;
        const std::string length(lengthStart, cursor);
        const char conversion = *cursor;
        if (conversion == '\0') return finish(assigned);
        std::string specifier(1, conversion);
        if (conversion == '[') {
            const char* setStart = cursor++;
            if (*cursor == '^') ++cursor;
            if (*cursor == ']') ++cursor;
            while (*cursor != '\0' && *cursor != ']') ++cursor;
            if (*cursor != ']') return finish(assigned);
            specifier.assign(setStart, cursor + 1);
        }
        ++cursor;
        if (conversion == 'n') {
            if (!suppress) {
                void* target = nextPointer();
                if (target == nullptr) return EOF;
                const auto count = input - buffer;
                if (length == "hh") *static_cast<signed char*>(target) = static_cast<signed char>(count);
                else if (length == "h") *static_cast<short*>(target) = static_cast<short>(count);
                else if (length.empty()) *static_cast<int*>(target) = static_cast<int>(count);
                else if (length == "l" || length == "ll" || length == "j" || length == "z" || length == "t") {
                    const std::int64_t value = count;
                    std::memcpy(target, &value, sizeof(value));
                } else throw std::invalid_argument("Invalid scan count length");
            }
            continue;
        }
#ifndef _WIN32
        const bool wide = conversion == 'S' || conversion == 'C' ||
            (length == "l" && (conversion == 's' || conversion == 'c' || conversion == '['));
        if (wide) {
            const char kind = conversion == 'S' ? 's' : (conversion == 'C' ? 'c' : conversion);
            char16_t* dest = nullptr;
            unsigned long unitBudget = ULONG_MAX;
            unsigned long pointBudget = width;
            if (kind == 'c' && pointBudget == 0) pointBudget = 1;
            if (!suppress) {
                void* raw = nextPointer();
                if (raw == nullptr) return finish(EOF);
                dest = static_cast<char16_t*>(raw);
                if (secure) {
                    const unsigned long capacity = static_cast<unsigned long>(nextCapacity());
                    if (capacity == 0) return finish(assigned);
                    if (kind == 'c') {
                        if (pointBudget > capacity) {
                            dest[0] = u'\0';
                            return finish(assigned);
                        }
                        unitBudget = capacity;
                    } else {
                        unitBudget = capacity - 1;
                    }
                }
            }
            std::vector<char32_t> points;
            const char* p = input;
            bool atEnd = false;
            if (kind == 's') {
                while (std::isspace(static_cast<unsigned char>(*p))) ++p;
                if (*p == '\0') atEnd = true;
                else {
                    unsigned used = 0;
                    while (*p != '\0' && !std::isspace(static_cast<unsigned char>(*p))) {
                        if (pointBudget != 0 && points.size() >= pointBudget) break;
                        const char* q = p;
                        char32_t code;
                        if (!DecodeUtf8(q, code)) break;
                        const unsigned units = code < 0x10000 ? 1u : 2u;
                        if (used + units > unitBudget) break;
                        points.push_back(code);
                        used += units;
                        p = q;
                    }
                }
            } else if (kind == 'c') {
                unsigned used = 0;
                for (unsigned long i = 0; i < pointBudget; ++i) {
                    if (*p == '\0') { atEnd = true; break; }
                    const char* q = p;
                    char32_t code;
                    if (!DecodeUtf8(q, code)) break;
                    const unsigned units = code < 0x10000 ? 1u : 2u;
                    if (used + units > unitBudget) break;
                    points.push_back(code);
                    used += units;
                    p = q;
                }
                if (points.empty()) return finish(!converted && atEnd ? EOF : assigned);
            } else {
                std::vector<char32_t> set;
                bool negated = false;
                ParseWideScanset(specifier, set, negated);
                unsigned used = 0;
                while (*p != '\0') {
                    if (pointBudget != 0 && points.size() >= pointBudget) break;
                    const char* q = p;
                    char32_t code;
                    if (!DecodeUtf8(q, code)) break;
                    bool inSet = false;
                    for (char32_t s : set) if (s == code) { inSet = true; break; }
                    if (inSet == negated) break;
                    const unsigned units = code < 0x10000 ? 1u : 2u;
                    if (used + units > unitBudget) break;
                    points.push_back(code);
                    used += units;
                    p = q;
                }
                if (points.empty()) return finish(!converted && *p == '\0' ? EOF : assigned);
            }
            if (points.empty()) return finish(!converted && atEnd ? EOF : assigned);
            if (!suppress) {
                char16_t* d = dest;
                for (char32_t code : points) d += EncodeUtf16(code, d);
                if (kind != 'c') *d = u'\0';
            }
            input = p;
            converted = true;
            if (!suppress) ++assigned;
            continue;
        }
#endif
        if (conversion != 'c' && conversion != '[') {
            const char* probe = input;
            while (std::isspace(static_cast<unsigned char>(*probe))) ++probe;
            if (*probe == '\0') return finish(converted ? assigned : EOF);
        }
        const bool sized = secure && (conversion == 's' || conversion == 'c' || conversion == '[');
        bool capped = false;
        std::string directive = "%";
        void* target = nullptr;
        if (suppress) directive += '*';
        else {
            target = nextPointer();
            if (target == nullptr) return EOF;
        }
        if (sized && !suppress) {
            const auto capacity = static_cast<unsigned long>(nextCapacity());
            const unsigned long limit = conversion == 'c' ? capacity : (capacity == 0 ? 0 : capacity - 1);
            const unsigned long wanted = conversion == 'c' ? (width == 0 ? 1 : width) : width;
            if (limit == 0 || (conversion == 'c' && wanted > limit)) {
                if (capacity != 0) static_cast<char*>(target)[0] = '\0';
                return finish(assigned);
            }
            capped = wanted == 0 || wanted > limit;
            width = wanted == 0 ? limit : std::min(wanted, limit);
        }
        if (width != 0) directive += std::to_string(width);
        directive += HostLength(length, conversion);
        directive += specifier;
        directive += "%n";
        int consumed = -1;
        const int matched = suppress ? HostSscanf(input, directive.c_str(), &consumed) : HostSscanf(input, directive.c_str(), target, &consumed);
        if (consumed < 0 || (!suppress && matched != 1)) return finish(!converted && *input == '\0' ? EOF : assigned);
        if (capped && conversion != 'c') {
            const char next = input[consumed];
            bool overflow = false;
            if (conversion == 's') overflow = next != '\0' && !std::isspace(static_cast<unsigned char>(next));
            else if (next != '\0') {
                char probe[2];
                const std::string test = "%1" + specifier;
                overflow = std::sscanf(input + consumed, test.c_str(), probe) == 1;
            }
            if (overflow) {
                static_cast<char*>(target)[0] = '\0';
                return finish(assigned);
            }
        }
        input += consumed;
        converted = true;
        if (!suppress) ++assigned;
    }
    return finish(assigned);
}

#ifndef _WIN32
struct FileScanInput {
    std::FILE* stream;
    std::string pushback;
    long consumed = 0;

    int get() {
        if (!pushback.empty()) {
            int c = static_cast<unsigned char>(pushback.back());
            pushback.pop_back();
            return c;
        }
        int c = std::fgetc(stream);
        if (c != EOF) ++consumed;
        return c;
    }
    void unget(int c) {
        if (c == EOF) return;
        pushback.push_back(static_cast<char>(c));
        --consumed;
    }
    void flush() {
        while (!pushback.empty()) {
            int c = static_cast<unsigned char>(pushback.back());
            pushback.pop_back();
            std::ungetc(c, stream);
        }
    }
};

bool DecodeUtf8Stream(FileScanInput& in, char32_t& code) {
    int b0 = in.get();
    if (b0 == EOF) return false;
    const unsigned char c = static_cast<unsigned char>(b0);
    if (c < 0x80) { code = c; return true; }
    unsigned length;
    char32_t value;
    if ((c & 0xe0) == 0xc0) { length = 2; value = c & 0x1f; }
    else if ((c & 0xf0) == 0xe0) { length = 3; value = c & 0x0f; }
    else if ((c & 0xf8) == 0xf0) { length = 4; value = c & 0x07; }
    else { in.unget(b0); return false; }
    unsigned char bytes[4];
    bytes[0] = c;
    for (unsigned i = 1; i < length; ++i) {
        int bi = in.get();
        if (bi == EOF) {
            for (unsigned j = i; j > 0; --j) in.unget(bytes[j - 1]);
            return false;
        }
        bytes[i] = static_cast<unsigned char>(bi);
        const unsigned char d = static_cast<unsigned char>(bi);
        if ((d & 0xc0) != 0x80) {
            for (unsigned j = i + 1; j > 0; --j) in.unget(bytes[j - 1]);
            return false;
        }
        value = (value << 6) | (d & 0x3f);
    }
    if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
        (length == 4 && value < 0x10000) || value > 0x10ffff ||
        (value >= 0xd800 && value <= 0xdfff)) {
        for (unsigned j = length; j > 0; --j) in.unget(bytes[j - 1]);
        return false;
    }
    code = value;
    return true;
}

template <typename NextPointer>
int ScanFileGuest(std::FILE* stream, const char* format, NextPointer nextPointer) {
    if (stream == nullptr || format == nullptr) return EOF;
    FileScanInput in{stream, {}, 0};
    int assigned = 0;
    bool converted = false;
    const char* cursor = format;
    const auto finish = [&](int result) {
        in.flush();
        return result;
    };
    while (*cursor != '\0') {
        if (std::isspace(static_cast<unsigned char>(*cursor))) {
            while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
            int c;
            do { c = in.get(); } while (c != EOF && std::isspace(static_cast<unsigned char>(c)));
            if (c != EOF) in.unget(c);
            continue;
        }
        if (*cursor != '%' || cursor[1] == '%') {
            const char expected = *cursor == '%' ? '%' : *cursor;
            cursor += *cursor == '%' ? 2 : 1;
            int c;
            if (expected == '%') {
                do { c = in.get(); } while (c != EOF && std::isspace(static_cast<unsigned char>(c)));
                if (c != EOF) in.unget(c);
            }
            c = in.get();
            if (c == EOF) return finish(!converted ? EOF : assigned);
            if (c != static_cast<unsigned char>(expected)) {
                in.unget(c);
                return finish(!converted ? EOF : assigned);
            }
            continue;
        }
        ++cursor;
        const bool suppress = *cursor == '*';
        if (suppress) ++cursor;
        unsigned long width = 0;
        while (std::isdigit(static_cast<unsigned char>(*cursor))) width = width * 10 + static_cast<unsigned long>(*cursor++ - '0');
        const char* lengthStart = cursor;
        while (std::strchr("hljztL", *cursor) != nullptr && *cursor != '\0') ++cursor;
        const std::string length(lengthStart, cursor);
        const char conversion = *cursor;
        if (conversion == '\0') return finish(assigned);
        std::string specifier(1, conversion);
        if (conversion == '[') {
            const char* setStart = cursor++;
            if (*cursor == '^') ++cursor;
            if (*cursor == ']') ++cursor;
            while (*cursor != '\0' && *cursor != ']') ++cursor;
            if (*cursor != ']') return finish(assigned);
            specifier.assign(setStart, cursor + 1);
        }
        ++cursor;
        if (conversion == 'n') {
            if (!suppress) {
                void* target = nextPointer();
                if (target == nullptr) return finish(EOF);
                const long count = in.consumed;
                if (length == "hh") *static_cast<signed char*>(target) = static_cast<signed char>(count);
                else if (length == "h") *static_cast<short*>(target) = static_cast<short>(count);
                else if (length.empty()) *static_cast<int*>(target) = static_cast<int>(count);
                else if (length == "l" || length == "ll" || length == "j" || length == "z" || length == "t") {
                    const std::int64_t value = count;
                    std::memcpy(target, &value, sizeof(value));
                } else throw std::invalid_argument("Invalid scan count length");
            }
            continue;
        }
        const bool wide = conversion == 'S' || conversion == 'C' ||
            (length == "l" && (conversion == 's' || conversion == 'c' || conversion == '['));
        if (wide) {
            const char kind = conversion == 'S' ? 's' : (conversion == 'C' ? 'c' : conversion);
            char16_t* dest = nullptr;
            unsigned long pointBudget = width;
            if (kind == 'c' && pointBudget == 0) pointBudget = 1;
            if (!suppress) {
                void* raw = nextPointer();
                if (raw == nullptr) return finish(EOF);
                dest = static_cast<char16_t*>(raw);
            }
            std::vector<char32_t> points;
            bool atEnd = false;
            if (kind == 's') {
                int c;
                do { c = in.get(); } while (c != EOF && std::isspace(static_cast<unsigned char>(c)));
                if (c == EOF) atEnd = true;
                else {
                    in.unget(c);
                    while (true) {
                        if (pointBudget != 0 && points.size() >= pointBudget) break;
                        int b = in.get();
                        if (b == EOF) { atEnd = true; break; }
                        if (std::isspace(static_cast<unsigned char>(b))) { in.unget(b); break; }
                        in.unget(b);
                        char32_t code;
                        if (!DecodeUtf8Stream(in, code)) break;
                        points.push_back(code);
                    }
                }
            } else if (kind == 'c') {
                for (unsigned long i = 0; i < pointBudget; ++i) {
                    char32_t code;
                    if (!DecodeUtf8Stream(in, code)) { atEnd = true; break; }
                    points.push_back(code);
                }
                if (points.empty()) return finish(!converted && atEnd ? EOF : assigned);
            } else {
                std::vector<char32_t> set;
                bool negated = false;
                ParseWideScanset(specifier, set, negated);
                while (true) {
                    if (pointBudget != 0 && points.size() >= pointBudget) break;
                    char32_t code;
                    if (!DecodeUtf8Stream(in, code)) { atEnd = true; break; }
                    bool inSet = false;
                    for (char32_t s : set) if (s == code) { inSet = true; break; }
                    if (inSet == negated) {
                        // Push back the bytes of this code point. Re-encode to UTF-8.
                        char tmp[4];
                        unsigned len = 0;
                        if (code < 0x80) { tmp[0] = static_cast<char>(code); len = 1; }
                        else if (code < 0x800) {
                            tmp[0] = static_cast<char>(0xc0 | (code >> 6));
                            tmp[1] = static_cast<char>(0x80 | (code & 0x3f));
                            len = 2;
                        } else if (code < 0x10000) {
                            tmp[0] = static_cast<char>(0xe0 | (code >> 12));
                            tmp[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3f));
                            tmp[2] = static_cast<char>(0x80 | (code & 0x3f));
                            len = 3;
                        } else {
                            tmp[0] = static_cast<char>(0xf0 | (code >> 18));
                            tmp[1] = static_cast<char>(0x80 | ((code >> 12) & 0x3f));
                            tmp[2] = static_cast<char>(0x80 | ((code >> 6) & 0x3f));
                            tmp[3] = static_cast<char>(0x80 | (code & 0x3f));
                            len = 4;
                        }
                        for (unsigned j = len; j > 0; --j) in.unget(static_cast<unsigned char>(tmp[j - 1]));
                        break;
                    }
                    points.push_back(code);
                }
                if (points.empty()) return finish(!converted && atEnd ? EOF : assigned);
            }
            if (points.empty()) return finish(!converted && atEnd ? EOF : assigned);
            if (!suppress) {
                char16_t* d = dest;
                for (char32_t code : points) d += EncodeUtf16(code, d);
                if (kind != 'c') *d = u'\0';
            }
            converted = true;
            if (!suppress) ++assigned;
            continue;
        }
        if (conversion != 'c' && conversion != '[') {
            int c;
            do { c = in.get(); } while (c != EOF && std::isspace(static_cast<unsigned char>(c)));
            if (c == EOF) return finish(converted ? assigned : EOF);
            in.unget(c);
        }
        in.flush();
        std::string directive = "%";
        void* target = nullptr;
        if (suppress) directive += '*';
        else {
            target = nextPointer();
            if (target == nullptr) return finish(EOF);
        }
        if (width != 0) directive += std::to_string(width);
        directive += HostLength(length, conversion);
        directive += specifier;
        directive += "%n";
        int hconsumed = -1;
        const int matched = suppress ?
            std::fscanf(stream, directive.c_str(), &hconsumed) :
            std::fscanf(stream, directive.c_str(), target, &hconsumed);
        if (hconsumed < 0 || (!suppress && matched != 1)) {
            return finish(matched == EOF && !converted ? EOF : assigned);
        }
        in.consumed += hconsumed;
        converted = true;
        if (!suppress) ++assigned;
    }
    return finish(assigned);
}
#endif

}

extern "C" {

int APS5_VABI vasprintf_nid_postfix(char** destination, const char* format, VaList* args) {
    if (!destination) { errno = 22; return -1; }
    *destination = nullptr;
    if (!format || !args) { errno = 22; return -1; }
    try {
#ifdef _WIN32
        std::string text;
        const int count = LibcDetail::FormatWindows(nullptr, 0, format, args, &text);
        const char* source = text.c_str();
#else
        std::string guestText;
        const bool guestWide = HasGuestWideFormat(format);
        char* text = nullptr;
        std::va_list copy;
        va_copy(copy, *reinterpret_cast<std::va_list*>(args));
        const int count = guestWide ? LibcDetail::FormatWindows(nullptr, 0, format, args, &guestText) : ::vasprintf(&text, format, copy);
        va_end(copy);
        std::unique_ptr<char, decltype(&std::free)> owner(text, std::free);
        if (count < 0) return -1;
        const char* source = guestWide ? guestText.c_str() : text;
#endif
        auto* output = static_cast<char*>(ApplicationHeapAllocate_nid_no_patch(static_cast<size_t>(count) + 1));
        if (!output) {
            errno = 12;
            return -1;
        }
        std::memcpy(output, source, static_cast<size_t>(count) + 1);
        *destination = output;
        return count;
    } catch (const std::bad_alloc&) {
        errno = 12;
        return -1;
    }
}

int APS5_VABI vfprintf_nid_postfix(FileStream* stream, const char* format, VaList* args) {
    if (stream && !stream->IsOpen()) { errno = 9; return -1; }
    auto* native = GetNativeStream(stream);
#ifdef _WIN32
    std::string buffer;
    const int count = LibcDetail::FormatWindows(nullptr, 0, format, args, &buffer);
    const int result = std::fwrite(buffer.data(), 1, static_cast<size_t>(count), native) ==
        static_cast<size_t>(count) ? count : -1;
#else
    if (HasGuestWideFormat(format)) {
        std::string buffer;
        const int count = LibcDetail::FormatWindows(nullptr, 0, format, args, &buffer);
        const int result = std::fwrite(buffer.data(), 1, static_cast<size_t>(count), native) == static_cast<size_t>(count) ? count : -1;
        stream->SyncStatus();
        return result;
    }
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(args));
    const int result = std::vfprintf(native, format, copy);
    va_end(copy);
#endif
    stream->SyncStatus();
    return result;
}

int APS5_VABI fprintf_nid_postfix(FileStream* stream, const char* format, ...) {
#ifdef _WIN32
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
#else
    std::va_list args;
    va_start(args, format);
#endif
    const int result = vfprintf_nid_postfix(stream, format, reinterpret_cast<VaList*>(args));
#ifdef _WIN32
    __builtin_sysv_va_end(args);
#else
    va_end(args);
#endif
    return result;
}

int APS5_VABI fscanf_nid_postfix(FileStream* stream, const char* format, ...) {
    if (stream && !stream->IsOpen()) { errno = 9; return -1; }
    auto* native = GetNativeStream(stream);
#ifdef _WIN32
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::ScanFileWindows_nid_no_patch(native, format, args);
    __builtin_sysv_va_end(args);
#else
    std::va_list args;
    va_start(args, format);
    const int result = HasWideScanFormat(format) ?
        ScanFileGuest(native, format, [&] { return va_arg(args, void*); }) :
        HostVfscanf(native, format, args);
    va_end(args);
#endif
    stream->SyncStatus();
    return result;
}

#ifdef _WIN32

int APS5_VABI printf_nid_postfix(const char* format, ...) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::PrintWindows(format, args);
    __builtin_sysv_va_end(args);
    return result;
}

int APS5_VABI libc_printf_nid_postfix(const char* format, ...) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::PrintWindows(format, args);
    __builtin_sysv_va_end(args);
    return result;
}

int APS5_VABI snprintf_nid_postfix(char* buffer, size_t size, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::FormatWindows(buffer, size, format, args);
    __builtin_sysv_va_end(args);
    return result;
}

int APS5_VABI sprintf_nid_postfix(char* buffer, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::FormatWindows(buffer, SIZE_MAX, format, args);
    __builtin_sysv_va_end(args);
    return result;
}

#else

int APS5_VABI printf_nid_postfix(const char* format, ...) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::PrintWindows(format, args) : std::vprintf(format, args);
    va_end(args);
    return result;
}

int APS5_VABI libc_printf_nid_postfix(const char* format, ...) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::PrintWindows(format, args) : std::vprintf(format, args);
    va_end(args);
    return result;
}

int APS5_VABI snprintf_nid_postfix(char* buffer, size_t size, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::FormatWindows(buffer, size, format, args) : std::vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

int APS5_VABI sprintf_nid_postfix(char* buffer, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::FormatWindows(buffer, SIZE_MAX, format, args) : std::vsprintf(buffer, format, args);
    va_end(args);
    return result;
}

#endif

int APS5_VABI vsscanf_nid_postfix(const char* input, const char* format, VaList* args) {
#ifdef _WIN32
    return LibcDetail::ScanWindows(input, format, args);
#else
    if (HasWideScanFormat(format)) {
        LibcDetail::FormatArguments arguments(args);
        return ScanGuest(input, format, false,
            [&] { return arguments.Next<void*>(); },
            [&] { return arguments.Next<unsigned int>(); });
    }
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(args));
    const int result = HostVsscanf(input, format, copy);
    va_end(copy);
    return result;
#endif
}

#ifdef _WIN32
int APS5_VABI sscanf_nid_postfix(const char* input, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = vsscanf_nid_postfix(input, format, reinterpret_cast<VaList*>(args));
    __builtin_sysv_va_end(args);
    return result;
}
#else
int APS5_VABI sscanf_nid_postfix(const char* input, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasWideScanFormat(format) ?
        ScanGuest(input, format, false,
            [&] { return va_arg(args, void*); },
            [&] { return va_arg(args, unsigned int); }) :
        HostVsscanf(input, format, args);
    va_end(args);
    return result;
}
#endif

#ifdef _WIN32

int APS5_VABI sscanf_s_nid_postfix(const char* buffer, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = ScanGuest(buffer, format, true, [&] { return __builtin_va_arg(args, void*); }, [&] { return __builtin_va_arg(args, unsigned int); });
    __builtin_sysv_va_end(args);
    return result;
}

#else

int APS5_VABI sscanf_s_nid_postfix(const char* buffer, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = ScanGuest(buffer, format, true, [&] { return va_arg(args, void*); }, [&] { return va_arg(args, unsigned int); });
    va_end(args);
    return result;
}

#endif

int APS5_VABI vprintf_nid_postfix(const char* str, VaList* c) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
#ifdef _WIN32
    return LibcDetail::PrintWindows(str, c);
#else
    if (HasGuestWideFormat(str)) return LibcDetail::PrintWindows(str, c);
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(c));
    const int result = std::vprintf(str, copy);
    va_end(copy);
    return result;
#endif
}

int APS5_VABI vsprintf_nid_postfix(char* str, const char* format, VaList* args) {
#ifdef _WIN32
    return LibcDetail::FormatWindows(str, SIZE_MAX, format, args);
#else
    if (HasGuestWideFormat(format)) return LibcDetail::FormatWindows(str, SIZE_MAX, format, args);
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(args));
    const int result = std::vsprintf(str, format, copy);
    va_end(copy);
    return result;
#endif
}

int APS5_VABI vsnprintf_nid_postfix(char* str, size_t size, const char* format, VaList* c) {
#ifdef _WIN32
    return LibcDetail::FormatWindows(str, size, format, c);
#else
    if (HasGuestWideFormat(format)) return LibcDetail::FormatWindows(str, size, format, c);
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(c));
    const int result = std::vsnprintf(str, size, format, copy);
    va_end(copy);
    return result;
#endif
}

#ifdef _WIN32

int APS5_VABI vsscanf_s_nid_postfix(const char* buffer, const char* format, VaList* args) {
    LibcDetail::FormatArguments arguments(args);
    return ScanGuest(buffer, format, true, [&] { return arguments.Next<void*>(); }, [&] { return arguments.Next<unsigned int>(); });
}

int APS5_VABI vsnprintf_s_nid_postfix(char* buffer, size_t size, const char* format, VaList* args) {
    return LibcDetail::FormatWindows(buffer, size, format, args);
}

int APS5_VABI snprintf_s_nid_postfix(char* buffer, size_t size, const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::FormatWindows(buffer, size, format, args);
    __builtin_sysv_va_end(args);
    return result;
}

int APS5_VABI printf_s_nid_postfix(const char* format, ...) {
    __builtin_sysv_va_list args;
    __builtin_sysv_va_start(args, format);
    const int result = LibcDetail::PrintWindows(format, args);
    __builtin_sysv_va_end(args);
    return result;
}

#else

int APS5_VABI vsscanf_s_nid_postfix(const char* buffer, const char* format, VaList* args) {
    LibcDetail::FormatArguments arguments(args);
    return ScanGuest(buffer, format, true, [&] { return arguments.Next<void*>(); }, [&] { return arguments.Next<unsigned int>(); });
}

int APS5_VABI vsnprintf_s_nid_postfix(char* buffer, size_t size, const char* format, VaList* args) {
    if (HasGuestWideFormat(format)) return LibcDetail::FormatWindows(buffer, size, format, args);
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(args));
    const int result = std::vsnprintf(buffer, size, format, copy);
    va_end(copy);
    return result;
}

int APS5_VABI vsprintf_s_nid_postfix(char* buffer, size_t size, const char* format, VaList* args) {
    if (HasGuestWideFormat(format)) return LibcDetail::FormatWindows(buffer, size, format, args);
    std::va_list copy;
    va_copy(copy, *reinterpret_cast<std::va_list*>(args));
    const int result = std::vsnprintf(buffer, size, format, copy);
    va_end(copy);
    return result;
}

int APS5_VABI sprintf_s_nid_postfix(char* buffer, size_t size, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::FormatWindows(buffer, size, format, args) : std::vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

int APS5_VABI snprintf_s_nid_postfix(char* buffer, size_t size, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::FormatWindows(buffer, size, format, args) : std::vsnprintf(buffer, size, format, args);
    va_end(args);
    return result;
}

int APS5_VABI printf_s_nid_postfix(const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    const int result = HasGuestWideFormat(format) ? LibcDetail::PrintWindows(format, args) : std::vprintf(format, args);
    va_end(args);
    return result;
}

#endif

int APS5_VABI puts_nid_postfix(const char* s) {
    if (!_Stdout_nid_postfix.IsOpen()) { errno = 9; return -1; }
    return std::puts(s);
}

}
