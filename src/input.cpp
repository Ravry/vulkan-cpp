#include "input.h"

namespace Input {
    std::vector<int> downKeys;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            downKeys.push_back(key);
            const char* name = glfwGetKeyName(key, scancode);
        } else if (action == GLFW_RELEASE) {
            auto it = std::find(downKeys.begin(), downKeys.end(), key);
            if (it != downKeys.end()) {
                downKeys.erase(it);
            }
        }
    }

    bool isKeyDown(int key) {
        auto it = std::find(downKeys.begin(), downKeys.end(), key);
        return it != downKeys.end();
    }
}