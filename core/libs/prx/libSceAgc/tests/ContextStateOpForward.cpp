#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/Shutdown.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

extern "C" std::uint32_t* APS5_VABI sceAgcDcbContextStateOp(CommandBuffer* buf, std::uint32_t operation);
extern "C" std::uint32_t* APS5_VABI sceAgcDcbContextStateOp_0100(CommandBuffer* buf, std::uint32_t operation);
extern "C" std::uint64_t APS5_VABI sceAgcDcbContextStateOpGetSize(std::uint32_t operation);

namespace {

void Require(bool value, const char* what) {
    if (value) return;
    std::puts(what);
    std::abort();
}

template <typename Call>
bool ThrowsRuntime(Call call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void CheckForwarding(std::uint32_t operation) {
    std::array<std::uint32_t, 64> forwardWords{};
    std::array<std::uint32_t, 64> directWords{};
    CommandBuffer forward{forwardWords.data(), forwardWords.data() + forwardWords.size(), forwardWords.data(),
                          forwardWords.data() + forwardWords.size(), nullptr, nullptr, 0};
    CommandBuffer direct{directWords.data(), directWords.data() + directWords.size(), directWords.data(),
                         directWords.data() + directWords.size(), nullptr, nullptr, 0};
    std::uint32_t* forwardResult = sceAgcDcbContextStateOp(&forward, operation);
    std::uint32_t* directResult = sceAgcDcbContextStateOp_0100(&direct, operation);
    Require(forwardResult == forwardWords.data(), "forwarding must return the start of the packet");
    Require(forwardResult - forwardWords.data() == directResult - directWords.data(),
            "forwarding must return the same offset as the direct call");
    Require(forward.cursor_up == forwardWords.data() + (direct.cursor_up - directWords.data()),
            "forwarding must advance the cursor like the direct call");
    Require(forwardWords == directWords, "forwarding must emit the same words as the direct call");
    const auto written = static_cast<std::uint64_t>(forward.cursor_up - forwardWords.data()) * sizeof(std::uint32_t);
    Require(written == sceAgcDcbContextStateOpGetSize(operation), "written size must match the reported size");
}

}

int main() {
    try {
        for (std::uint32_t operation = 0; operation < 4; ++operation) CheckForwarding(operation);
        std::array<std::uint32_t, 64> words{};
        CommandBuffer buffer{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(),
                             nullptr, nullptr, 0};
        Require(ThrowsRuntime([&] { sceAgcDcbContextStateOp(nullptr, 0); }), "null buffer must throw");
        Require(ThrowsRuntime([&] { sceAgcDcbContextStateOp(&buffer, 4); }), "operation 4 must throw");
        LibcRunShutdown_nid_postfix();
        std::puts("AGC context state op forwarding tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::puts(error.what());
        return 1;
    }
}
