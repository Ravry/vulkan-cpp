#include "vulkan-base.h"

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage) {
    VulkanSwapchain result = {};
    VkBool32 supportsPresent = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, surface, &supportsPresent);
    if (!supportsPresent) {
        printf("graphics queue does not support present");
        return result;
    }

    uint32_t numFormats {0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, 0);
    std::vector<VkSurfaceFormatKHR> availableFormats;
    availableFormats.resize(numFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats.data());
    if (numFormats <= 0)
    {
        printf("no surface formats available");
        return result;
    }

    VkFormat format = availableFormats[0].format;
    VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities);
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
        surfaceCapabilities.currentExtent.width = surfaceCapabilities.minImageExtent.width;
    }
    if (surfaceCapabilities.currentExtent.height == 0xFFFFFFFF) {
        surfaceCapabilities.currentExtent.height = surfaceCapabilities.minImageExtent.height;
    }

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = 3;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = usage;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    // createInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(context->device, &createInfo, 0, &result.swapchain);

    result.format = format;
    result.width = surfaceCapabilities.currentExtent.width;
    result.height = surfaceCapabilities.currentExtent.height;

    uint32_t numImages;
    vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, 0);
    result.images.resize(numImages);
    vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, result.images.data());

    result.imageViews.resize(numImages);
    for (size_t i {0}; i < numImages; i++) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = result.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.components =  {};
        createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCreateImageView(context->device, &createInfo, 0, &result.imageViews[i]);
    }

    return result;
}

void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain) {
    for (size_t i {0} ; i < swapchain->imageViews.size(); i++) {
        vkDestroyImageView(context->device, swapchain->imageViews[i], 0);
    }
    vkDestroySwapchainKHR(context->device, swapchain->swapchain, 0);
}