#pragma once
#include <iostream>
#include <string_view>
#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>
#include "vulkan-base.h"

class Window {
private: 
    GLFWwindow* window;
    
    VulkanContext* context;
    VkSurfaceKHR surface;
    VulkanSwapchain swapchain;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    VulkanPipeline pipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
    VkSemaphore acquireSemaphore;
    VkSemaphore releaseSemaphore;

    uint16_t width;
    uint16_t height;
public:
    Window(const uint16_t width, const uint16_t height, const std::string_view title);
    void setupVulkan();
    void run();
    void render();
    void clean();
};