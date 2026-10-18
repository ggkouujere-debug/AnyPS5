#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

struct Packet {
    std::uint32_t* addr;
    std::uint32_t dw_num;
    std::uint8_t flags;
    std::uint8_t reserved[3];
};

extern "C" {
int APS5_VABI sceAgcDriverSubmitCommandBuffer(void* queue_context, const Packet* packet);
int APS5_VABI sceAgcDriverSubmitMultiCommandBuffers(void* queue_context, uint32_t* const* command_buffers, const uint32_t* sizes_in_dwords, uint32_t count);
}

namespace AgcDriver {
int SubmitCallCount = 0;
void Submit(const Packet* packet, std::uint32_t queue) {
    (void)packet;
    (void)queue;
    ++SubmitCallCount;
}
}

namespace {

constexpr int ErrorInval = -22;

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

}

int main() {
    std::uint32_t dummy[4] = {0, 0, 0, 0};
    Packet nullAddr{};
    nullAddr.addr = nullptr;
    nullAddr.dw_num = 4;
    Packet zeroDwords{};
    zeroDwords.addr = dummy;
    zeroDwords.dw_num = 0;

    Require(sceAgcDriverSubmitCommandBuffer(nullptr, nullptr) == ErrorInval, "null packet must return EINVAL");
    Require(sceAgcDriverSubmitCommandBuffer(nullptr, &nullAddr) == ErrorInval, "null addr must return EINVAL");
    Require(sceAgcDriverSubmitCommandBuffer(nullptr, &zeroDwords) == ErrorInval, "zero dwords must return EINVAL");

    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, nullptr, nullptr, 0) == ErrorInval, "null arrays must return EINVAL");
    std::uint32_t* buffers[1] = {dummy};
    std::uint32_t sizes[1] = {4};
    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, nullptr, sizes, 1) == ErrorInval, "null buffers must return EINVAL");
    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, buffers, nullptr, 1) == ErrorInval, "null sizes must return EINVAL");
    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, buffers, sizes, 0) == 0, "zero count must return 0");

    Packet valid{};
    valid.addr = dummy;
    valid.dw_num = 4;
    int before = AgcDriver::SubmitCallCount;
    Require(sceAgcDriverSubmitCommandBuffer(nullptr, &valid) == 0, "valid packet must return 0");
    Require(AgcDriver::SubmitCallCount == before + 1, "valid packet must call Driver::Submit once");

    before = AgcDriver::SubmitCallCount;
    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, buffers, sizes, 1) == 0, "valid multi must return 0");
    Require(AgcDriver::SubmitCallCount == before + 1, "valid multi must call Driver::Submit once");

    std::uint32_t badSizes[1] = {0};
    Require(sceAgcDriverSubmitMultiCommandBuffers(nullptr, buffers, badSizes, 1) == ErrorInval, "zero-size entry must return EINVAL");
    Require(AgcDriver::SubmitCallCount == before + 1, "failed multi must not call Driver::Submit again");

    std::puts("Submit bridge validation tests passed");
    return 0;
}
