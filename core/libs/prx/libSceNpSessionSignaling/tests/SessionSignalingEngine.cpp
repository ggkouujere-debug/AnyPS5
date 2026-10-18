#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
int APS5_VABI sceNpSessionSignalingGetMemoryInfo(void);
int APS5_VABI sceNpSessionSignalingGetConnectionStatistics(void);
int APS5_VABI sceNpSessionSignalingGetConnectionFromPeerAddress(void);
int APS5_VABI sceNpSessionSignalingGetGroupFromSessionId(void);
int APS5_VABI sceNpSessionSignalingGetGroupInfo();
int APS5_VABI sceNpSessionSignalingGetConnectionFromNetAddress2();
[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName);
}

[[noreturn]] void NotImplemented_nid_no_patch(const char* funcName) {
    std::puts(funcName);
    std::puts("NotImplemented called for an implemented function");
    std::abort();
}

namespace {

constexpr int ErrorUnavailable = static_cast<int>(0x80552D06u);

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

}

int main() {
    Require(sceNpSessionSignalingGetMemoryInfo() == ErrorUnavailable, "GetMemoryInfo must return UNAVAILABLE");
    Require(sceNpSessionSignalingGetConnectionStatistics() == ErrorUnavailable, "GetConnectionStatistics must return UNAVAILABLE");
    Require(sceNpSessionSignalingGetConnectionFromPeerAddress() == ErrorUnavailable, "GetConnectionFromPeerAddress must return UNAVAILABLE");
    Require(sceNpSessionSignalingGetGroupFromSessionId() == ErrorUnavailable, "GetGroupFromSessionId must return UNAVAILABLE");
    Require(sceNpSessionSignalingGetGroupInfo() == ErrorUnavailable, "GetGroupInfo must return UNAVAILABLE");
    Require(sceNpSessionSignalingGetConnectionFromNetAddress2() == ErrorUnavailable, "GetConnectionFromNetAddress2 must return UNAVAILABLE");
    std::puts("SessionSignaling engine tests passed");
    return 0;
}
