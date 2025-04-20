#pragma once
#include <algorithm>
#include <vector>
#include <GLFW/glfw3.h>

namespace Input {
    extern std::vector<int> downKeys;
    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    bool isKeyDown(int key);
}