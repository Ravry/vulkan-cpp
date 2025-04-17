#include "vulkan-base.h"

VkShaderModule createShaderModule(VulkanContext* context, const char* shaderFilename) {
    VkShaderModule result = {};

    FILE* file = fopen(shaderFilename, "rb");
    if (!file)
    {
        LOG(LOG_ERROR_UTILS, "shader not found: %s", shaderFilename);
        return result;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* buffer = new uint8_t[fileSize];
    fread(buffer, 1, fileSize, file);
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = fileSize;
    createInfo.pCode = (uint32_t*)buffer;
    VAC(vkCreateShaderModule(context->device, &createInfo, 0, &result));
    delete[] buffer;
    fclose(file);
    return result;
}

void createPipeline(VulkanContext* context, const char* vertexShaderFilename, const char* fragmentShaderFilename, VkRenderPass renderPass, uint32_t width, uint32_t height, VulkanPipeline& pipeline) {
    VkShaderModule vertexShaderModule = createShaderModule(context, vertexShaderFilename);
    VkShaderModule fragmentShaderModule = createShaderModule(context, fragmentShaderFilename);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.resize(2);
    shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    VkViewport viewport = {0.0f, 0.0f, (float)width, (float)height};
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    VkRect2D scissor = {{0,0}, {width, height}};
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    VkPipelineLayout pipelineLayout;
    {
        VkPipelineLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        VAC(vkCreatePipelineLayout(context->device, &createInfo, 0, &pipelineLayout));
    }

    VkPipeline _pipeline;
    {
        VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        createInfo.stageCount = shaderStages.size();
        createInfo.pStages = shaderStages.data();
        createInfo.pVertexInputState = &vertexInputState;
        createInfo.pInputAssemblyState = &inputAssemblyState;
        createInfo.pViewportState = &viewportState;
        createInfo.pRasterizationState = &rasterizationState;
        createInfo.pMultisampleState = &multisampleState;
        createInfo.pColorBlendState = &colorBlendState;
        createInfo.layout = pipelineLayout;
        createInfo.renderPass = renderPass;
        createInfo.subpass = 0;
        VAC(vkCreateGraphicsPipelines(context->device, 0, 1, &createInfo, 0, &_pipeline));
    }

    vkDestroyShaderModule(context->device, vertexShaderModule, 0);
    vkDestroyShaderModule(context->device, fragmentShaderModule, 0);

    pipeline = {};
    pipeline.pipeline = _pipeline;
    pipeline.pipelineLayout = pipelineLayout;
}

void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline) {
    vkDestroyPipeline(context->device, pipeline->pipeline, 0);
    vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, 0);
}