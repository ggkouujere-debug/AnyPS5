#include "prx/libSceAgcDriver/Submit/include/CommandBuffer.hpp"

#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace AgcDriver {
void Submit(const Packet* packet, std::uint32_t queue);
}

namespace {

constexpr int ErrorInval = -22;
constexpr int ErrorIo = -5;

int SubmitOne(const Packet* packet) {
    if (packet == nullptr || packet->addr == nullptr || packet->dw_num == 0)
        return ErrorInval;
    try {
        AgcDriver::Submit(packet, 0);
    } catch (...) {
        return ErrorIo;
    }
    return 0;
}

}

extern "C" {

int APS5_VABI sceAgcDriverSubmitCommandBuffer(void* queue_context, const Packet* packet) {
    (void)queue_context;
    return SubmitOne(packet);
}

int APS5_VABI sceAgcDriverSubmitMultiCommandBuffers(void* queue_context, uint32_t* const* command_buffers, const uint32_t* sizes_in_dwords, uint32_t count) {
    (void)queue_context;
    if (command_buffers == nullptr || sizes_in_dwords == nullptr)
        return ErrorInval;
    for (uint32_t i = 0; i < count; ++i) {
        Packet packet{};
        packet.addr = command_buffers[i];
        packet.dw_num = sizes_in_dwords[i];
        int result = SubmitOne(&packet);
        if (result != 0)
            return result;
    }
    return 0;
}

}
