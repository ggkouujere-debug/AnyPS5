#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" {
void APS5_VABI sceNpRegisterGamePresenceCallback(void* callback, void* userdata);
int APS5_VABI sceNpSetContentRestriction(const void* restriction);
[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName);
}

[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName) {
    std::puts(funcName);
    std::puts("NotImplemented called for an implemented function");
    std::abort();
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

void DummyCallback() {}

}

int main() {
    bool threw = false;
    try {
        sceNpRegisterGamePresenceCallback(nullptr, nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw, "null callback must throw invalid_argument");

    sceNpRegisterGamePresenceCallback(reinterpret_cast<void*>(&DummyCallback), nullptr);

    Require(sceNpSetContentRestriction(nullptr) == 0, "null restriction must return 0");

    std::puts("NpManager engine tests passed");
    return 0;
}
