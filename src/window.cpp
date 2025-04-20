#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#include "window.h"

Window::Window(const uint16_t width, const uint16_t height, const std::string_view title) : width(width), height(height) {    
    if (!glfwInit()) {
        throw std::runtime_error("error while initializing glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    
    glfwSetKeyCallback(window, Input::keyCallback);

    setupVulkan();
}

void Window::setupVulkan() {
    initVulkan(context);

    VAC(glfwCreateWindowSurface(context->instance, window, nullptr, &surface));

    createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapchain);    
    createRenderPass(context, swapchain.format, renderPass);
    createFramebuffers(context, swapchain, renderPass, framebuffers);
    createPipeline(context, "spvs/default-vert.spv", "spvs/default-frag.spv", renderPass, swapchain.width, swapchain.height, pipeline);
    createFence(context, fence);
    createSemaphore(context, acquireSemaphore);
    createSemaphore(context, releaseSemaphore);
    createCommandPool(context, &commandPool);
    allocateCommandBuffers(context, commandPool, commandBuffer);
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
            LOG(LOG_DEFAULT_UTILS, 0, "FPS: %f (%fms)", 1.0f/deltaTime, deltaTime * 1000.f);
            fpsTimer = .0;
        }

        glfwPollEvents();
        
        {
            if (Input::isKeyDown(GLFW_KEY_ESCAPE))
                glfwSetWindowShouldClose(window, true);
        }
        
        render();
    }
    clean();
}

uint32_t frameIndex = 0;

void Window::render() {
    uint32_t imageIndex {0};

    vkWaitForFences(context->device, 1, &fence[frameIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(context->device, 1, &fence[frameIndex]);

    vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphore[frameIndex], 0, &imageIndex);

    // vkResetCommandPool(context->device, commandPool, 0);
    vkResetCommandBuffer(commandBuffer[frameIndex], 0);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer[frameIndex], &beginInfo);
    {
        VkClearValue clearValue = {1.0f, 0.0f, 1.0f, 1.0f};
        VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = framebuffers[imageIndex];
        beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height}};
        beginInfo.clearValueCount = 1;
        beginInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(commandBuffer[frameIndex], &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(commandBuffer[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdDraw(commandBuffer[frameIndex], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer[frameIndex]);
    }
    vkEndCommandBuffer(commandBuffer[frameIndex]);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer[frameIndex];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore[frameIndex];
    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore[frameIndex];
    vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fence[frameIndex]);

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore[frameIndex];

    vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Window::clean() {
    vkDeviceWaitIdle(context->device);
    destroySyncObjects(context, acquireSemaphore, releaseSemaphore, fence);
    vkDestroyCommandPool(context->device, commandPool, 0);

    destroyPipeline(context, &pipeline);
    destroyFramebuffers(context, framebuffers);
    destroyRenderpass(context, renderPass);
    destroySwapchain(context, &swapchain);
    
    vkDestroySurfaceKHR(context->instance, surface, 0);
    
    cleanVulkan(context);

    glfwDestroyWindow(window);
    glfwTerminate();
}