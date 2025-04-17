#include "vulkan-base.h"

void dumpValidationLayers() {
    uint32_t layerPropertyCount;
    VAC(vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0));
    std::vector<VkLayerProperties> layerProperties;
    layerProperties.resize(layerPropertyCount);
    VAC(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()));

    for (size_t i {0}; i < layerPropertyCount; i++) {
        LOG(LOG_DEFAULT_UTILS, false, "layer-property-name: %s", layerProperties[i].layerName);
        LOG(LOG_DEFAULT_UTILS, false, "layer-property-description: %s", layerProperties[i].description);
    }
};

void dumpInstanceExtensions() {
    uint32_t instanceExtensionCount;
    VAC(vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, 0));
    std::vector<VkExtensionProperties> instanceExtensionProperties;
    instanceExtensionProperties.resize(instanceExtensionCount);
    VAC(vkEnumerateInstanceExtensionProperties(0, &instanceExtensionCount, instanceExtensionProperties.data()));

    for (size_t i {0}; i < instanceExtensionCount; i++) {
        LOG(LOG_DEFAULT_UTILS, false, "instance-extension-name: %s", instanceExtensionProperties[i].extensionName);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        throw std::runtime_error("validation layer: " + std::string(pCallbackData->pMessage));
    }
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkDebugUtilsMessengerEXT debugMessenger;

void populateCustomDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void createCustomDebugMessenger(VulkanContext* context) {    
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    populateCustomDebugMessengerCreateInfo(createInfo);
    VAC(CreateDebugUtilsMessengerEXT(context->instance, &createInfo, nullptr, &debugMessenger));
}

bool initVulkanInstance(VulkanContext* context) {
    // dumpValidationLayers();
    // dumpInstanceExtensions();

    std::vector<const char*> enabledLayers = {
        "VK_LAYER_KHRONOS_validation"
        // "VK_LAYER_LUNARG_monitor"
    };
    
    uint32_t glfwInstanceExtensionCount;
    const char** glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&glfwInstanceExtensionCount);
    std::vector<const char*> enabledExtensions (glfwInstanceExtensions, glfwInstanceExtensions + glfwInstanceExtensionCount);

    for (size_t i {0}; i < glfwInstanceExtensionCount; i++) {
        LOG(LOG_DEFAULT_UTILS, false, "glfw-extension-name: %s", glfwInstanceExtensions[i]);
    }
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "vulkan engine";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    applicationInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = enabledLayers.size();
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.enabledExtensionCount = enabledExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    populateCustomDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    VAC(vkCreateInstance(&createInfo, 0, &context->instance));

    createCustomDebugMessenger(context);

    return true;
}

bool selectPhysicalDevice(VulkanContext* context) {
    uint32_t numDevices {0};
    VAC(vkEnumeratePhysicalDevices(context->instance, &numDevices, 0));

    if (numDevices == 0) {
        return false;
    }

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(numDevices);
    VAC(vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices.data()));

    for (auto& physicalDevice : physicalDevices) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        LOG(LOG_DEFAULT_UTILS, false, "physical-device-name: %s", properties.deviceName);
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

    VAC(vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device));

    context->graphicsQueue.familyIndex = graphicsQueueIndex;
    vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue);

    return true;
}

void initVulkan(VulkanContext*& context) {
    context = new VulkanContext;
    
    if (!initVulkanInstance(context)) {
        LOG(LOG_ERROR_UTILS, false, "error creating vulkan instance");
    }

    if (!selectPhysicalDevice(context)) {
        LOG(LOG_ERROR_UTILS, false, "error finding physical device");
    }
    
    if (!createLogicalDevice(context)) {
        LOG(LOG_ERROR_UTILS, false, "errror creating logical device");
    }
}

void cleanVulkan(VulkanContext*& context) {
    vkDeviceWaitIdle(context->device),
    vkDestroyDevice(context->device, 0);
    DestroyDebugUtilsMessengerEXT(context->instance, debugMessenger, nullptr);
    vkDestroyInstance(context->instance, 0);
    delete context;
    context = nullptr;
}

void createFramebuffers(VulkanContext* context, std::vector<VkFramebuffer>& framebuffers, VulkanSwapchain& swapchain, VkRenderPass& renderPass) {
    framebuffers.resize(swapchain.images.size());
    for (size_t i {0}; i < swapchain.images.size(); i++) {
        VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapchain.imageViews[i];
        createInfo.width = swapchain.width;
        createInfo.height = swapchain.height;
        createInfo.layers = 1;
        VAC(vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]));
    }
}

void destroyFramebuffers(VulkanContext* context, std::vector<VkFramebuffer>& framebuffers) {
    for (size_t i {0}; i < framebuffers.size(); i++) {
        vkDestroyFramebuffer(context->device, framebuffers[i], 0);
    }
    framebuffers.clear();
}

void createFence(VulkanContext* context, VkFence* fence) {
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VAC(vkCreateFence(context->device, &createInfo, 0, fence));
}

void createSemaphore(VulkanContext* context, VkSemaphore* semaphore) {
    VkSemaphoreCreateInfo createInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VAC(vkCreateSemaphore(context->device, &createInfo, 0, semaphore));
}

void createCommandPool(VulkanContext* context, VkCommandPool* commandPool) {
    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
    VAC(vkCreateCommandPool(context->device, &createInfo, 0, commandPool));
}

void allocateCommandBuffers(VulkanContext* context, VkCommandPool& commandPool, VkCommandBuffer* commandBuffer) {
    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    VAC(vkAllocateCommandBuffers(context->device, &allocateInfo, commandBuffer));
}