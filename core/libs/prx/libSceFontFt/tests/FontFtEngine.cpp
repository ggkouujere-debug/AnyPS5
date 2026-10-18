#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceFontFtSupportBdf();
int APS5_VABI sceFontFtSupportCid();
int APS5_VABI sceFontFtSupportFontFormats();
int APS5_VABI sceFontFtSupportOpenType();
int APS5_VABI sceFontFtSupportOpenTypeOtf();
int APS5_VABI sceFontFtSupportOpenTypeTtf();
int APS5_VABI sceFontFtSupportPcf();
int APS5_VABI sceFontFtSupportPfr();
int APS5_VABI sceFontFtSupportSystemFonts();
int APS5_VABI sceFontFtSupportTrueType();
int APS5_VABI sceFontFtSupportTrueTypeGx();
int APS5_VABI sceFontFtSupportType1();
int APS5_VABI sceFontFtSupportType42();
int APS5_VABI sceFontFtSupportWinFonts();
[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName);
}

[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName) {
    std::puts(funcName);
    std::puts("NotImplemented called for an implemented function");
    std::abort();
}

namespace Font {
struct SysDriver {};
}
namespace FontFt {
const Font::SysDriver* DriverTable() { return nullptr; }
const Font::SysDriver* RendererTable() { return nullptr; }
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

}

int main() {
    Require(sceFontFtSupportBdf() == 1, "BDF support must return 1");
    Require(sceFontFtSupportCid() == 1, "CID support must return 1");
    Require(sceFontFtSupportFontFormats() == 1, "FontFormats support must return 1");
    Require(sceFontFtSupportOpenType() == 1, "OpenType support must return 1");
    Require(sceFontFtSupportOpenTypeOtf() == 1, "OpenTypeOtf support must return 1");
    Require(sceFontFtSupportOpenTypeTtf() == 1, "OpenTypeTtf support must return 1");
    Require(sceFontFtSupportPcf() == 1, "PCF support must return 1");
    Require(sceFontFtSupportPfr() == 1, "PFR support must return 1");
    Require(sceFontFtSupportSystemFonts() == 1, "SystemFonts support must return 1");
    Require(sceFontFtSupportTrueType() == 1, "TrueType support must return 1");
    Require(sceFontFtSupportTrueTypeGx() == 1, "TrueTypeGx support must return 1");
    Require(sceFontFtSupportType1() == 1, "Type1 support must return 1");
    Require(sceFontFtSupportType42() == 1, "Type42 support must return 1");
    Require(sceFontFtSupportWinFonts() == 1, "WinFonts support must return 1");
    std::puts("FontFt engine tests passed");
    return 0;
}
