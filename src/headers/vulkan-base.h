#pragma once
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "logger.h"

struct VulkanQueue {
    VkQueue queue;
    uint32_t familyIndex;
};

struct VulkanSwapchain {
    VkSwapchainKHR swapchain;
    uint32_t width;
    uint32_t height;
    VkFormat format;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};

struct VulkanPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VulkanQueue graphicsQueue;
};

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);

VulkanContext* initVulkan();
void cleanVulkan(VulkanContext* context);