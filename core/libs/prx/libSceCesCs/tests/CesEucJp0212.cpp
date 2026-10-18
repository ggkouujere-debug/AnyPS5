#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
int APS5_VABI sceCesEucJpToUcs2(const void* profile, const std::uint8_t* euc, std::uint32_t eucMax, std::uint32_t* eucSize,
                                std::uint16_t* ucs2);
void* APS5_VABI sceCesUcsProfileInitEucJpCp51932(void* sheet);
int APS5_VABI sceCesMbcsUcsContextInit(void* context, const void* profile);
int APS5_VABI sceCesMbcsStrToUtf8Str(void* context, const std::uint8_t* mbcs, std::uint32_t mbcsMax, std::uint32_t* mbcsLen,
                                     std::uint8_t* utf8, std::uint32_t utf8max, std::uint32_t* utf8Len);
int APS5_VABI sceCesMbcsStrGetUtf8Len(void* context, const std::uint8_t* mbcs, std::uint32_t mbcsMax,
                                      std::uint32_t* mbcsLen, std::uint32_t* utf8Len);
}

namespace {

constexpr int ErrorInvalidParameter = static_cast<int>(0x805C0001u);
constexpr int ErrorInvalidSrcBuffer = static_cast<int>(0x805C0010u);
constexpr int ErrorSrcBufferEnd = static_cast<int>(0x805C0011u);
constexpr int ErrorInvalidEncode = static_cast<int>(0x805C0014u);
constexpr int ErrorUnassignedCode = static_cast<int>(0x805C0020u);
constexpr int ErrorInvalidDstBuffer = static_cast<int>(0x805C0030u);

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

void RequireEucJpToUcs2(const void* profile, const std::uint8_t* euc, std::uint32_t eucMax, std::uint16_t expected) {
    std::uint16_t ucs2 = 0;
    std::uint32_t size = 0;
    Require(sceCesEucJpToUcs2(profile, euc, eucMax, &size, &ucs2) == 0, "JIS X 0212 conversion must succeed");
    Require(size == 3, "JIS X 0212 must consume 3 bytes");
    Require(ucs2 == expected, "JIS X 0212 produced the wrong code point");
}

}

int main() {
    std::uint8_t sheet[8]{};
    const void* profile = sceCesUcsProfileInitEucJpCp51932(sheet);
    Require(profile != nullptr, "the EUC-JP profile must initialize");

    const std::uint8_t breve[] = {0x8F, 0xA2, 0xAF};
    const std::uint8_t caron[] = {0x8F, 0xA2, 0xB0};
    const std::uint8_t macron[] = {0x8F, 0xA2, 0xB4};
    RequireEucJpToUcs2(profile, breve, sizeof(breve), 0x02D8);
    RequireEucJpToUcs2(profile, caron, sizeof(caron), 0x02C7);
    RequireEucJpToUcs2(profile, macron, sizeof(macron), 0x00AF);

    std::uint16_t ucs2 = 0;
    std::uint32_t size = 0;
    Require(sceCesEucJpToUcs2(nullptr, breve, sizeof(breve), &size, &ucs2) == ErrorInvalidParameter,
            "null profile must return CES_ERROR_INVALID_PARAMETER");
    Require(sceCesEucJpToUcs2(profile, nullptr, sizeof(breve), &size, &ucs2) == ErrorInvalidSrcBuffer,
            "null source must return CES_ERROR_INVALID_SRC_BUFFER");
    Require(sceCesEucJpToUcs2(profile, breve, 0, &size, &ucs2) == ErrorSrcBufferEnd,
            "zero length must return CES_ERROR_SRC_BUFFER_END");
    Require(sceCesEucJpToUcs2(profile, breve, sizeof(breve), &size, nullptr) == ErrorInvalidDstBuffer,
            "null destination must return CES_ERROR_INVALID_DST_BUFFER");
    const std::uint8_t truncated[] = {0x8F, 0xA2};
    Require(sceCesEucJpToUcs2(profile, truncated, sizeof(truncated), &size, &ucs2) == ErrorSrcBufferEnd,
            "truncated plane must return CES_ERROR_SRC_BUFFER_END");
    const std::uint8_t badTrail[] = {0x8F, 0xA2, 0x20};
    Require(sceCesEucJpToUcs2(profile, badTrail, sizeof(badTrail), &size, &ucs2) == ErrorInvalidEncode,
            "bad trail byte must return CES_ERROR_INVALID_ENCODE");
    const std::uint8_t unassigned[] = {0x8F, 0xA1, 0xA1};
    Require(sceCesEucJpToUcs2(profile, unassigned, sizeof(unassigned), &size, &ucs2) == ErrorUnassignedCode,
            "unassigned cell must return CES_ERROR_UNASSIGNED_CODE");

    std::uint64_t contextStorage = 0;
    Require(sceCesMbcsUcsContextInit(&contextStorage, profile) == 0, "context init with EUC-JP must succeed");
    const std::uint8_t text[] = {0x8F, 0xA2, 0xAF, 0x00};
    std::uint8_t utf8[8]{};
    std::uint32_t mbcsLen = 0;
    std::uint32_t utf8Len = 0;
    Require(sceCesMbcsStrToUtf8Str(&contextStorage, text, sizeof(text), &mbcsLen, utf8, sizeof(utf8), &utf8Len) == 0,
            "JIS X 0212 string conversion must succeed");
    Require(mbcsLen == 3 && utf8Len == 2, "JIS X 0212 string conversion must report the right lengths");
    Require(std::memcmp(utf8, "\xCB\x98", 2) == 0, "JIS X 0212 string conversion produced the wrong bytes");
    Require(sceCesMbcsStrGetUtf8Len(&contextStorage, text, sizeof(text), &mbcsLen, &utf8Len) == 0 && utf8Len == 2,
            "JIS X 0212 length query must report 2 UTF-8 bytes");

    std::puts("CES JIS X 0212 tests passed");
    return 0;
}
