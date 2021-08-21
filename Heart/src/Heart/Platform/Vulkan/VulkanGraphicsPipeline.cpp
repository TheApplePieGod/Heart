#include "htpch.h"
#include "VulkanGraphicsPipeline.h"

#include "Heart/Platform/Vulkan/VulkanShader.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"

namespace Heart
{
    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, VkRenderPass renderPass, VkSampleCountFlagBits sampleCount)
        : GraphicsPipeline(createInfo)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            VulkanCommon::DefineShaderStage(static_cast<VulkanShader*>(createInfo.VertexShader.get())->GetShaderModule(), VK_SHADER_STAGE_VERTEX_BIT),
            VulkanCommon::DefineShaderStage(static_cast<VulkanShader*>(createInfo.FragmentShader.get())->GetShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        // auto bindingDescription = pipeline.pipelineInfo.getVertexBindingDescription();
        // auto attributeDescriptions = pipeline.pipelineInfo.getVertexAttributeDescriptions();

        // VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        // vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // vertexInputInfo.vertexBindingDescriptionCount = 1;
        // vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        // vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
        // vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        // VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        // inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        // inputAssembly.topology = VulkanCommon::VertexTopologyToVulkan(createInfo.VertexTopology);
        // inputAssembly.primitiveRestartEnable = VK_FALSE;

        // VkPipelineViewportStateCreateInfo viewportState{};
        // viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        // viewportState.viewportCount = 0;
        // viewportState.pViewports = nullptr;
        // viewportState.scissorCount = 0;
        // viewportState.pScissors = nullptr;

        // VkPipelineRasterizationStateCreateInfo rasterizer{};
        // rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        // rasterizer.depthClampEnable = VK_FALSE;
        // rasterizer.rasterizerDiscardEnable = VK_FALSE;
        // rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        // rasterizer.lineWidth = 1.0f;
        // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        // rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        // rasterizer.depthBiasEnable = VK_FALSE;
        // rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        // rasterizer.depthBiasClamp = 0.0f; // Optional
        // rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // VkPipelineMultisampleStateCreateInfo multisampling{};
        // multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        // multisampling.sampleShadingEnable = VK_FALSE;
        // multisampling.rasterizationSamples = sampleCount;
        // multisampling.minSampleShading = 1.0f; // Optional
        // multisampling.pSampleMask = nullptr; // Optional
        // multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        // multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // colorBlendAttachment.blendEnable = VK_TRUE;
        // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // VkPipelineColorBlendStateCreateInfo colorBlending{};
        // colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        // colorBlending.logicOpEnable = VK_FALSE;
        // colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        // colorBlending.attachmentCount = 1;
        // colorBlending.pAttachments = &colorBlendAttachment;
        // colorBlending.blendConstants[0] = 0.0f; // Optional
        // colorBlending.blendConstants[1] = 0.0f; // Optional
        // colorBlending.blendConstants[2] = 0.0f; // Optional
        // colorBlending.blendConstants[3] = 0.0f; // Optional

        // VkDynamicState dynamicStates[] = {
        //     VK_DYNAMIC_STATE_VIEWPORT,
        //     VK_DYNAMIC_STATE_SCISSOR
        // };
        // VkPipelineDynamicStateCreateInfo dynamicState{};
        // dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        // dynamicState.dynamicStateCount = 2;
        // dynamicState.pDynamicStates = dynamicStates;

        // // VkPushConstantRange pushConstants{};
        // // pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        // // pushConstants.offset = 0;
        // // pushConstants.size = sizeof(diamond_object_data);

        // VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        // pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // pipelineLayoutInfo.setLayoutCount = 1;
        // pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        // pipelineLayoutInfo.pushConstantRangeCount = 0;
        // pipelineLayoutInfo.pPushConstantRanges = nullptr;
        // HE_VULKAN_CHECK_RESULT(vkCreatePipelineLayout(device.Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

        // VkPipelineDepthStencilStateCreateInfo depthStencil{};
        // depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        // depthStencil.depthTestEnable = createInfo.DepthEnable;
        // depthStencil.depthWriteEnable = createInfo.DepthEnable;
        // depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        // depthStencil.depthBoundsTestEnable = VK_FALSE;
        // depthStencil.minDepthBounds = 0.0f; // Optional
        // depthStencil.maxDepthBounds = 1.0f; // Optional
        // depthStencil.stencilTestEnable = VK_FALSE;
        // depthStencil.front = {}; // Optional
        // depthStencil.back = {}; // Optional

        // VkGraphicsPipelineCreateInfo pipelineInfo{};
        // pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        // pipelineInfo.stageCount = 2;
        // pipelineInfo.pStages = shaderStages;
        // pipelineInfo.pVertexInputState = &vertexInputInfo;
        // pipelineInfo.pInputAssemblyState = &inputAssembly;
        // pipelineInfo.pViewportState = &viewportState;
        // pipelineInfo.pRasterizationState = &rasterizer;
        // pipelineInfo.pMultisampleState = &multisampling;
        // pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        // pipelineInfo.pColorBlendState = &colorBlending;
        // pipelineInfo.pDynamicState = &dynamicState; // Optional
        // pipelineInfo.layout = m_PipelineLayout;
        // pipelineInfo.renderPass = renderPass;
        // pipelineInfo.subpass = 0;
        // pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        // pipelineInfo.basePipelineIndex = -1; // Optional
        // HE_VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(device.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyPipeline(device.Device(), m_Pipeline, nullptr);
        vkDestroyPipelineLayout(device.Device(), m_PipelineLayout, nullptr);
    }
}