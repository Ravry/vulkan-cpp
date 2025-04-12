#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#include "window.h"

Window::Window(const uint16_t width, const uint16_t height, const std::string_view title) : width(width), height(height) {
    if (!glfwInit()) {
        printf("error while initializing glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    
    context = initVulkan();
    if (glfwCreateWindowSurface(context->instance, window, nullptr, &surface) != VK_SUCCESS) {
        printf("error creating glfw window surface");
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

    pipeline = createPipeline(context, "../shaders/default-vert.spv", "../shaders/default-frag.spv", renderPass, swapchain.width, swapchain.height);

    {
        VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        vkCreateFence(context->device, &createInfo, 0, &fence);
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
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        uint32_t imageIndex {0};
        vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, 0, fence, &imageIndex);

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

        vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context->device, 1, &fence);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, 0);

        vkDeviceWaitIdle(context->device);

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);
    }
    clean();
}

void Window::clean() {
    vkDeviceWaitIdle(context->device);
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