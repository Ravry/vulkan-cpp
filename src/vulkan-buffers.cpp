#include "vulkan-base.h"

uint32_t findMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("failed to find suitable memory type!");
}

void createVertexBuffer(VulkanContext* context, const std::vector<Vertex>& vertices, VkBuffer* vertexBuffer, VkDeviceMemory* vertexBufferMemory) {
    VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VAC(vkCreateBuffer(context->device, &bufferInfo, nullptr, vertexBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context->device, *vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VAC(vkAllocateMemory(context->device, &allocInfo, nullptr, vertexBufferMemory));

    vkBindBufferMemory(context->device, *vertexBuffer, *vertexBufferMemory, 0);

    void* data;
    vkMapMemory(context->device, *vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferInfo.size);
    vkUnmapMemory(context->device, *vertexBufferMemory);
}

void destroyVertexBuffer(VulkanContext* context, VkBuffer& vertexBuffer) {
    vkDestroyBuffer(context->device, vertexBuffer, 0);
}