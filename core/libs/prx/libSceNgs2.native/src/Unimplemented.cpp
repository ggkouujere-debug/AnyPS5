#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>

#include "prx/libc/include/General.hpp"
#include "prx/libSceNgs2.native/include/Ngs2Types.hpp"

#pragma GCC visibility push(default)

namespace {

struct ReportHandler {
    std::uint32_t reportType;
    void* handler;
    std::uintptr_t userData;
};

std::mutex reportMutex;
std::map<std::uintptr_t, ReportHandler> reportHandlers;
std::uintptr_t nextReportHandle{1};

}

extern "C" {

int APS5_VABI sceNgs2ReportRegisterHandler(uint32_t report_type, void* handler, uintptr_t user_data, Ngs2Handle* out_handle) {
    if (handler == nullptr) throw std::runtime_error(std::string(__func__) + ": null handler");
    if (out_handle == nullptr) APS5_INVALID_ARG_EX;
    std::lock_guard lock(reportMutex);
    const auto handle = nextReportHandle++;
    reportHandlers.emplace(handle, ReportHandler{report_type, handler, user_data});
    *out_handle = handle;
    return 0;
}

int APS5_VABI sceNgs2ReportUnregisterHandler(Ngs2Handle report_handle) {
    std::lock_guard lock(reportMutex);
    if (reportHandlers.erase(report_handle) == 0) throw std::runtime_error(std::string(__func__) + ": unknown report handle");
    return 0;
}

}

#pragma GCC visibility pop
