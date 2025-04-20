#pragma once
#include <string_view>
#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>
#include "vulkan-base.h"
#include "input.h"

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
    std::vector<VkCommandBuffer> commandBuffer;
    std::vector<VkFence> fence;
    std::vector<VkSemaphore> acquireSemaphore;
    std::vector<VkSemaphore> releaseSemaphore;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

public:
    uint16_t width;
    uint16_t height;
    bool framebufferResized {false};

    Window(const uint16_t width, const uint16_t height, const std::string_view title);
    void setupVulkan();
    void run();
    void render();
    void clean();
};