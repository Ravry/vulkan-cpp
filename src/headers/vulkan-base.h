#pragma once
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "logger.h"

#define VAC(value) \
    do { \
        if ((value) < 0) { \
            throw std::runtime_error("vulkan-related exception!"); \
        } \
    } while(0)


const size_t MAX_FRAMES_IN_FLIGHT = 2;

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

void createFramebuffers(VulkanContext* context, VulkanSwapchain& swapchain, VkRenderPass& renderPass, std::vector<VkFramebuffer>& framebuffers);
void destroyFramebuffers(VulkanContext* context, std::vector<VkFramebuffer>& framebuffers);

void createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height, VulkanPipeline& pipeline);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);

void createFence(VulkanContext* context, std::vector<VkFence>& fences);
void createSemaphore(VulkanContext* context, std::vector<VkSemaphore>& semaphores);
void createCommandPool(VulkanContext* context, VkCommandPool* commandPool);
void destroySyncObjects(VulkanContext* context, std::vector<VkSemaphore>& acquireSemaphores, std::vector<VkSemaphore>& releaseSemaphores, std::vector<VkFence>& fences);

void allocateCommandBuffers(VulkanContext* context, VkCommandPool& commandPool, std::vector<VkCommandBuffer>& commandBuffer);