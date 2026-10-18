#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" {
void APS5_VABI __tsan_acquire(void*);
void APS5_VABI __tsan_release(void*);
}

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

}

int main() {
    __tsan_acquire(nullptr);
    __tsan_release(nullptr);
    std::uint64_t canary = 0x0123456789ABCDEFULL;
    std::uint64_t* slot = &canary;
    __tsan_acquire(slot);
    __tsan_release(slot);
    Require(canary == 0x0123456789ABCDEFULL, "tsan hooks must not touch the address");
    for (int i = 0; i < 1024; ++i) {
        __tsan_acquire(slot);
        __tsan_release(slot);
    }
    Require(canary == 0x0123456789ABCDEFULL, "tsan hooks must stay side-effect free");
    std::puts("ThreadSanitizer no-op tests passed");
    return 0;
}
