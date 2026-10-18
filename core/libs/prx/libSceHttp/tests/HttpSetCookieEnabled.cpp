#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceHttpSetCookieEnabled(int id, int enable);
}

namespace {

constexpr int ErrorInvalidValue = static_cast<int>(0x804311FEu);

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

}

int main() {
    Require(sceHttpSetCookieEnabled(1, 0) == 0, "disabling cookies must return 0");
    Require(sceHttpSetCookieEnabled(42, 0) == 0, "disabling cookies must return 0 for any id");
    Require(sceHttpSetCookieEnabled(1, 1) == 0, "enabling cookies must return 0");
    Require(sceHttpSetCookieEnabled(1, 2) == ErrorInvalidValue, "enable=2 must return ERROR_INVALID_VALUE");
    Require(sceHttpSetCookieEnabled(1, -1) == ErrorInvalidValue, "enable=-1 must return ERROR_INVALID_VALUE");
    Require(sceHttpSetCookieEnabled(1, 100) == ErrorInvalidValue, "enable=100 must return ERROR_INVALID_VALUE");
    Require(sceHttpSetCookieEnabled(1, -2147483647 - 1) == ErrorInvalidValue, "enable=INT_MIN must return ERROR_INVALID_VALUE");
    std::puts("HTTP set cookie enabled tests passed");
    return 0;
}
