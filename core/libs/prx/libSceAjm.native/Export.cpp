#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <atomic>
#include <cstdio>

// Entry points this library does not implement: reported before the unimplemented-call exception so an
// APS5_TRACE_AJM run shows which one a title reached.
static void AjmStub(const char* name) {
    std::fprintf(stderr, "[ajm] unimplemented %s called\n", name);
    NotImplemented_nid_no_patch(name);
}

namespace {

constexpr int AjmErrorInvalidContext = static_cast<int>(0x80930002);
constexpr int AjmErrorInvalidInstance = static_cast<int>(0x80930003);
constexpr int AjmErrorInvalidBatch = static_cast<int>(0x80930004);
constexpr int AjmErrorInvalidParameter = static_cast<int>(0x80930005);
constexpr int AjmErrorOutOfResources = static_cast<int>(0x80930007);

}

extern "C" {

int APS5_VABI sceAjmBatchJobEncode(AjmBatchInfo* info, uint32_t instance, const void* pcm_input, size_t pcm_input_size, void* bitstream_output, size_t bitstream_output_size, void* result) {
 (void)info;
 (void)instance;
 (void)pcm_input;
 (void)pcm_input_size;
 (void)bitstream_output;
 (void)bitstream_output_size;
 (void)result;
 AjmStub(__func__);
 return 0;
}

const char* APS5_VABI sceAjmStrError(int error) {
    switch (error) {
        case AjmErrorInvalidContext: return "SCE_AJM_ERROR_INVALID_CONTEXT";
        case AjmErrorInvalidInstance: return "SCE_AJM_ERROR_INVALID_INSTANCE";
        case AjmErrorInvalidBatch: return "SCE_AJM_ERROR_INVALID_BATCH";
        case AjmErrorInvalidParameter: return "SCE_AJM_ERROR_INVALID_PARAMETER";
        case AjmErrorOutOfResources: return "SCE_AJM_ERROR_OUT_OF_RESOURCES";
        default: break;
    }
    thread_local char unknown[48];
    std::snprintf(unknown, sizeof(unknown), "unknown AJM error %#x", static_cast<unsigned int>(error));
    return unknown;
}

int APS5_VABI sceAjmDecWVorbisCreateHeaderPacket(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceAjmDecWVorbisCreateSetupPacket(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
