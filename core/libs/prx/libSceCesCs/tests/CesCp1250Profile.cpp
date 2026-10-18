#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
const void* APS5_VABI sceCesRefersUcsProfileCp1250(void);
const std::uint8_t* APS5_VABI sceCesRefersUcsProfileCp1252(void);
int APS5_VABI sceCesSbcToUtf8(const std::uint8_t* profile, std::uint8_t sbc, std::uint8_t* utf8, std::uint32_t utf8max,
                              std::uint32_t* utf8_len);
int APS5_VABI sceCesMbcsUcsContextInit(void* context, const void* profile);
int APS5_VABI sceCesMbcsStrToUtf8Str(void* context, const std::uint8_t* mbcs, std::uint32_t mbcsMax, std::uint32_t* mbcsLen,
                                     std::uint8_t* utf8, std::uint32_t utf8max, std::uint32_t* utf8Len);
}

namespace {

constexpr int ErrorInvalidParameter = static_cast<int>(0x805C0001u);
constexpr int ErrorInvalidDstBuffer = static_cast<int>(0x805C0030u);
constexpr int ErrorDstBufferEnd = static_cast<int>(0x805C0031u);

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

void RequireSbcToUtf8(const std::uint8_t* profile, std::uint8_t sbc, const char* expected, std::uint32_t expectedLen) {
    std::uint8_t out[4]{};
    std::uint32_t produced = 0;
    Require(sceCesSbcToUtf8(profile, sbc, out, sizeof(out), &produced) == 0, "CP1250 conversion must succeed");
    Require(produced == expectedLen, "CP1250 conversion produced the wrong length");
    Require(std::memcmp(out, expected, expectedLen) == 0, "CP1250 conversion produced the wrong bytes");
}

}

int main() {
    const void* profile = sceCesRefersUcsProfileCp1250();
    Require(profile != nullptr, "the CP1250 profile must exist");
    Require(profile != sceCesRefersUcsProfileCp1252(), "the CP1250 profile must differ from CP1252");
    const auto* sbcProfile = static_cast<const std::uint8_t*>(profile);

    RequireSbcToUtf8(sbcProfile, 0xA3, "\xC5\x81", 2);
    RequireSbcToUtf8(sbcProfile, 0xB3, "\xC5\x82", 2);
    RequireSbcToUtf8(sbcProfile, 0x41, "A", 1);
    RequireSbcToUtf8(sbcProfile, 0x80, "\xE2\x82\xAC", 3);
    RequireSbcToUtf8(sbcProfile, 0xFF, "\xCB\x99", 2);

    std::uint8_t out[4]{};
    std::uint32_t produced = 0;
    Require(sceCesSbcToUtf8(nullptr, 0xA3, out, sizeof(out), &produced) == ErrorInvalidParameter,
            "null profile must return CES_ERROR_INVALID_PARAMETER");
    Require(sceCesSbcToUtf8(sbcProfile, 0xA3, nullptr, sizeof(out), &produced) == ErrorInvalidDstBuffer,
            "null destination must return CES_ERROR_INVALID_DST_BUFFER");
    Require(sceCesSbcToUtf8(sbcProfile, 0xA3, out, 1, &produced) == ErrorDstBufferEnd,
            "short destination must return CES_ERROR_DST_BUFFER_END");

    std::uint64_t contextStorage = 0;
    Require(sceCesMbcsUcsContextInit(&contextStorage, profile) == 0, "context init with CP1250 must succeed");
    const std::uint8_t mbcs[] = {0xA3, 0xB3, 0x00};
    std::uint8_t utf8[8]{};
    std::uint32_t mbcsLen = 0;
    std::uint32_t utf8Len = 0;
    Require(sceCesMbcsStrToUtf8Str(&contextStorage, mbcs, sizeof(mbcs), &mbcsLen, utf8, sizeof(utf8), &utf8Len) == 0,
            "CP1250 string conversion must succeed");
    Require(mbcsLen == 2 && utf8Len == 4, "CP1250 string conversion must report the right lengths");
    Require(std::memcmp(utf8, "\xC5\x81\xC5\x82", 4) == 0, "CP1250 string conversion produced the wrong bytes");

    std::puts("CES CP1250 profile tests passed");
    return 0;
}
