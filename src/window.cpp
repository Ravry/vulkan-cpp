#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#include "window.h"

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = false;
    if (width > 0 && height > 0) {
        app->width = (uint16_t)width;
        app->height = (uint16_t)height;    
    }
}

Window::Window(const uint16_t width, const uint16_t height, const std::string_view title) : width(width), height(height) {    
    if (!glfwInit()) {
        throw std::runtime_error("error while initializing glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    
    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, Input::keyCallback);

    setupVulkan();

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    createVertexBuffer(context, vertices, &vertexBuffer, &vertexBufferMemory);
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

    VkResult result = vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphore[frameIndex], 0, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        framebufferResized = false;
        recreateSwapchain(window, context, swapchain, framebuffers, surface, renderPass);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image!");
    }    

    vkResetFences(context->device, 1, &fence[frameIndex]);

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
        
        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffer[frameIndex], 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset = {0, 0};
        scissor.extent = {width, height};
        vkCmdSetScissor(commandBuffer[frameIndex], 0, 1, &scissor);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer[frameIndex], 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer[frameIndex], static_cast<uint32_t>(3), 1, 0, 0);

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

    result = vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain(window, context, swapchain, framebuffers, surface, renderPass);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain image!");
    }   

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Window::clean() {
    vkDeviceWaitIdle(context->device);
    
    destroyVertexBuffer(context, vertexBuffer);
    vkFreeMemory(context->device, vertexBufferMemory, 0);

    destroySwapchain(context, &swapchain, framebuffers);

    destroyPipeline(context, &pipeline);
    destroyRenderpass(context, renderPass);

    destroySyncObjects(context, acquireSemaphore, releaseSemaphore, fence);
    vkDestroyCommandPool(context->device, commandPool, 0);
    
    vkDestroySurfaceKHR(context->instance, surface, 0);
    
    cleanVulkan(context);

    glfwDestroyWindow(window);
    glfwTerminate();
}