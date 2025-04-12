#include "window.h"
#include "vulkan-base.h"

int main() {
    enableAnsiColors();
    Window window(800, 600, "Vulkan Engine");
    window.run();
    return 0;
}