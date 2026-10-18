#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>

extern "C" int APS5_VABI sceSaveDataCancel();

int main() {
    const int result = sceSaveDataCancel();
    if (result != 0) {
        std::fprintf(stderr, "sceSaveDataCancel returned 0x%08x instead of SAVE_DATA_OK\n", result);
        return 1;
    }
    std::puts("sceSaveDataCancel tests passed");
    return 0;
}
