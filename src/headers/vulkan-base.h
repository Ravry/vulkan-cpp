#pragma once
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "logger.h"

#define VAC(value, handler) \
    do { \
        if ((value) < 0) { \
            LOG(LOG_ERROR_UTILS, "an error occurred!"); \
            handler; \
        } \
    } while(0)

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

void initVulkan(VulkanContext*& context);
void cleanVulkan(VulkanContext*& context);

void createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain& swapchain);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

void createRenderPass(VulkanContext* context, VkFormat format, VkRenderPass& renderPass);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);

void createFramebuffers(VulkanContext* context, std::vector<VkFramebuffer>& framebuffers, VulkanSwapchain& swapchain, VkRenderPass& renderPass);
void destroyFramebuffers(VulkanContext* context, std::vector<VkFramebuffer>& framebuffers);

void createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height, VulkanPipeline& pipeline);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);

void createFence(VulkanContext* context, VkFence* fence);

void createSemaphore(VulkanContext* context, VkSemaphore* semaphore);

void createCommandPool(VulkanContext* context, VkCommandPool* commandPool);

void allocateCommandBuffers(VulkanContext* context, VkCommandPool& commandPool, VkCommandBuffer* commandBuffer);