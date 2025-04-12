#include "vulkan-base.h"

void dumpValidationLayers() {
    uint32_t layerPropertyCount;
    vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0);
    std::vector<VkLayerProperties> layerProperties;
    layerProperties.resize(layerPropertyCount);
    vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data());

    for (size_t i {0}; i < layerPropertyCount; i++) {
        LOG(LOG_DEFAULT_UTILS, "layer-property-name: %s", layerProperties[i].layerName);
        LOG(LOG_DEFAULT_UTILS, "layer-property-description: %s", layerProperties[i].description);
    }
};

void dumpInstanceExtensions() {
    uint32_t instanceExtensionCount;
    vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, 0);
    std::vector<VkExtensionProperties> instanceExtensionProperties;
    instanceExtensionProperties.resize(instanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, instanceExtensionProperties.data());

    for (size_t i {0}; i < instanceExtensionCount; i++) {
        LOG(LOG_DEFAULT_UTILS, "instance-extension-name: %s", instanceExtensionProperties[i].extensionName);
    }
}

bool initVulkanInstance(VulkanContext* context) {
    // dumpValidationLayers();
    
    std::vector<const char*> enabledLayers = {
        "VK_LAYER_KHRONOS_validation",
        // "VK_LAYER_LUNARG_monitor"
    };

    // dumpInstanceExtensions();
    
    uint32_t glfwInstanceExtensionCount;
    const char** glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&glfwInstanceExtensionCount);
    
    for (size_t i {0}; i < glfwInstanceExtensionCount; i++) {
        LOG(LOG_DEFAULT_UTILS, "glfw-extension-name: %s", glfwInstanceExtensions[i]);
    }

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "vulkan engine";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    applicationInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = enabledLayers.size();
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.enabledExtensionCount = glfwInstanceExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwInstanceExtensions;
    
    if (vkCreateInstance(&createInfo, 0, &context->instance) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool selectPhysicalDevice(VulkanContext* context) {
    uint32_t numDevices {0};
    vkEnumeratePhysicalDevices(context->instance, &numDevices, 0);

    if (numDevices == 0) {
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numDevices);
    vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices.data());

    for (auto& physicalDevice : physicalDevices) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        LOG(LOG_DEFAULT_UTILS, "physical-device-name: %s", properties.deviceName);
    }

    context->physicalDevice = physicalDevices[0];
    return true;
}

bool createLogicalDevice(VulkanContext* context) {
    uint32_t numQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, 0);
    std::vector<VkQueueFamilyProperties> queueFamilies;
    queueFamilies.resize(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies.data());

    uint32_t graphicsQueueIndex {0};
    for (size_t i {0}; i < queueFamilies.size(); i++) {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];
        if (queueFamily.queueCount > 0) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueIndex = i;
                break;
            }
        }
    }

    float priorities[] = { 1.f };
    VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = priorities;

    VkPhysicalDeviceFeatures enabledFeatures = {};
    
    std::vector<const char*> enabledDeviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = enabledDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    createInfo.pEnabledFeatures = &enabledFeatures;

    if (vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device)) {
        return false;
    }

    context->graphicsQueue.familyIndex = graphicsQueueIndex;
    vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue);

    return true;
}

bool initVulkan(VulkanContext*& context) {
    context = new VulkanContext;

    if (!initVulkanInstance(context)) {
        LOG(LOG_ERROR_UTILS, "error creating vulkan instance");
        return false;
    }

    if (!selectPhysicalDevice(context)) {
        LOG(LOG_ERROR_UTILS, "error finding physical device");
        return false;
    }
    
    if (!createLogicalDevice(context)) {
        LOG(LOG_ERROR_UTILS, "errror creating logical device");
        return false;
    }

    return true;
}

void cleanVulkan(VulkanContext* context) {
    vkDeviceWaitIdle(context->device),
    vkDestroyDevice(context->device, 0);
    vkDestroyInstance(context->instance, 0);
}