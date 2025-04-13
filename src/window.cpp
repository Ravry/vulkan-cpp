#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#include "window.h"

Window::Window(const uint16_t width, const uint16_t height, const std::string_view title) : width(width), height(height) {
    if (!glfwInit()) {
        LOG(LOG_ERROR_UTILS, "error while initializing glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    
    setupVulkan();
}

void Window::setupVulkan() {
    if (!initVulkan(context)) {
        LOG(LOG_DEFAULT_UTILS, "error while initializing vulkan");
    }

    if (glfwCreateWindowSurface(context->instance, window, nullptr, &surface) != VK_SUCCESS) {
        LOG(LOG_ERROR_UTILS, "error creating glfw window surface");
        return;
    }
    
    swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);    
    renderPass = createRenderPass(context, swapchain.format);

    framebuffers.resize(swapchain.images.size());
    for (size_t i {0}; i < swapchain.images.size(); i++) {
        VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapchain.imageViews[i];
        createInfo.width = swapchain.width;
        createInfo.height = swapchain.height;
        createInfo.layers = 1;
        vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]);
    }

    pipeline = createPipeline(context, "spvs/default-vert.spv", "spvs/default-frag.spv", renderPass, swapchain.width, swapchain.height);

    {
        VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(context->device, &createInfo, 0, &fence);
    }

    {
        VkSemaphoreCreateInfo createInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphore);
        vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphore);
    }

    {
        VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
        vkCreateCommandPool(context->device, &createInfo, 0, &commandPool);
    }

    {
        VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer);
    }
}

void Window::run() {
    double deltaTime {.0};
    double lastTime {.0};
    double elapsedTime {.0};
    double fpsTimer {.0};

    while(!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        elapsedTime += deltaTime;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.) {
            LOG(LOG_DEFAULT_UTILS, "%f", 1.f/deltaTime);
            fpsTimer = .0;
        }

        glfwPollEvents();
        render();
    }
    clean();
}

void Window::render() {
    uint32_t imageIndex {0};

    vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(context->device, 1, &fence);

    vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphore, 0, &imageIndex);

    vkResetCommandPool(context->device, commandPool, 0);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    {
        VkClearValue clearValue = {1.0f, 0.0f, 1.0f, 1.0f};
        VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = framebuffers[imageIndex];
        beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height}};
        beginInfo.clearValueCount = 1;
        beginInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore;
    vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fence);

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;

    vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);
}

void Window::clean() {
    vkDeviceWaitIdle(context->device);
    vkDestroySemaphore(context->device, acquireSemaphore, 0);
    vkDestroySemaphore(context->device, releaseSemaphore, 0);
    vkDestroyFence(context->device, fence, 0);
    vkDestroyCommandPool(context->device, commandPool, 0);
    destroyPipeline(context, &pipeline);
    for (size_t i {0}; i < framebuffers.size(); i++) {
        vkDestroyFramebuffer(context->device, framebuffers[i], 0);
    }
    framebuffers.clear();
    destroyRenderpass(context, renderPass);
    destroySwapchain(context, &swapchain);
    vkDestroySurfaceKHR(context->instance, surface, 0);
    cleanVulkan(context);
    delete context;
    context = nullptr;
    glfwTerminate();
}