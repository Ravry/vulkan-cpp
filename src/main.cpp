#include "window.h"
#include "vulkan-base.h"

int main() {
    enableAnsiColors();
    try {
        Window window(800, 600, "Vulkan Engine");
        window.run();
    } catch(std::exception& exception) {
        LOG(LOG_ERROR_UTILS, "std::exception: %s", exception.what());
    }
    return 0;
}