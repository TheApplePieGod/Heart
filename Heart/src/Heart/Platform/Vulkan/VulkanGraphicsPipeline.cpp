#include "hepch.h"
#include "VulkanGraphicsPipeline.h"

#include "Heart/Platform/Vulkan/VulkanShader.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Renderer/Renderer.h"

namespace Heart
{
    static VkVertexInputBindingDescription GenerateVertexBindingDescription(const BufferLayout& layout)
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = layout.GetStride();
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static HVector<VkVertexInputAttributeDescription> GenerateVertexAttributeDescriptions(const HVector<BufferLayoutElement>& elements)
    {
        HVector<VkVertexInputAttributeDescription> attributeDescriptions(elements.Count());

        u32 offset = 0;
        for (u32 i = 0; i < elements.Count(); i++)
        {
            attributeDescriptions[i].binding = 0;
            attributeDescriptions[i].location = static_cast<u32>(i);
            attributeDescriptions[i].format = VulkanCommon::BufferDataTypeToVulkan(elements[i].DataType);
            attributeDescriptions[i].offset = offset;
            offset += elements[i].CalculatedSize;
        }
        
        return attributeDescriptions;
    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, VkRenderPass renderPass, VkSampleCountFlagBits sampleCount, u32 viewportWidth, u32 viewportHeight)
        : GraphicsPipeline(createInfo)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        m_DescriptorSet.Initialize(m_ProgramReflectionData);

        auto vertShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.VertexShaderAsset)->GetShader();
        auto fragShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.FragmentShaderAsset)->GetShader();
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            VulkanCommon::DefineShaderStage(static_cast<VulkanShader*>(vertShader)->GetShaderModule(), VK_SHADER_STAGE_VERTEX_BIT),
            VulkanCommon::DefineShaderStage(static_cast<VulkanShader*>(fragShader)->GetShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        auto bindingDescription = GenerateVertexBindingDescription(createInfo.VertexLayout);
        auto attributeDescriptions = GenerateVertexAttributeDescriptions(createInfo.VertexLayout.GetElements());

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = createInfo.VertexInput ? 1 : 0;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = createInfo.VertexInput ? static_cast<u32>(attributeDescriptions.Count()) : 0;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.Data();
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VulkanCommon::VertexTopologyToVulkan(createInfo.VertexTopology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)viewportWidth;
        viewport.height = (f32)viewportHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = { viewportWidth, viewportHeight };

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VulkanCommon::CullModeToVulkan(createInfo.CullMode);
        rasterizer.frontFace = VulkanCommon::WindingOrderToVulkan(createInfo.WindingOrder);
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = sampleCount;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        HVector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(createInfo.BlendStates.Count());
        for (u32 i = 0; i < createInfo.BlendStates.Count(); i++)
        {
            colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachments[i].blendEnable = createInfo.BlendStates[i].BlendEnable;
            colorBlendAttachments[i].srcColorBlendFactor = VulkanCommon::BlendFactorToVulkan(createInfo.BlendStates[i].SrcColorBlendFactor);
            colorBlendAttachments[i].dstColorBlendFactor = VulkanCommon::BlendFactorToVulkan(createInfo.BlendStates[i].DstColorBlendFactor);
            colorBlendAttachments[i].colorBlendOp = VulkanCommon::BlendOperationToVulkan(createInfo.BlendStates[i].ColorBlendOperation);
            colorBlendAttachments[i].srcAlphaBlendFactor = VulkanCommon::BlendFactorToVulkan(createInfo.BlendStates[i].SrcAlphaBlendFactor);
            colorBlendAttachments[i].dstAlphaBlendFactor = VulkanCommon::BlendFactorToVulkan(createInfo.BlendStates[i].DstAlphaBlendFactor);
            colorBlendAttachments[i].alphaBlendOp = VulkanCommon::BlendOperationToVulkan(createInfo.BlendStates[i].AlphaBlendOperation);
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = static_cast<u32>(colorBlendAttachments.Count());
        colorBlending.pAttachments = colorBlendAttachments.Data();
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 3;
        dynamicState.pDynamicStates = dynamicStates;

        // VkPushConstantRange pushConstants{};
        // pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        // pushConstants.offset = 0;
        // pushConstants.size = sizeof(diamond_object_data);

        HVector<VkDescriptorSetLayout> layouts;
        layouts.AddInPlace(m_DescriptorSet.GetLayout());

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<u32>(layouts.Count());
        pipelineLayoutInfo.pSetLayouts = layouts.Data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        HE_VULKAN_CHECK_RESULT(vkCreatePipelineLayout(device.Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = createInfo.DepthTest;
        depthStencil.depthWriteEnable = createInfo.DepthWrite;
        depthStencil.depthCompareOp = Renderer::IsUsingReverseDepth() ? VK_COMPARE_OP_GREATER_OR_EQUAL : VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState; // Optional
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = createInfo.SubpassIndex;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional
        HE_VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(device.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        m_DescriptorSet.Shutdown();

        auto pipeline = m_Pipeline;
        auto layout = m_PipelineLayout;

        Renderer::PushJobQueue([=]()
        {
            VulkanDevice& device = VulkanContext::GetDevice();

            vkDestroyPipeline(device.Device(), pipeline, nullptr);
            vkDestroyPipelineLayout(device.Device(), layout, nullptr);
        });
    }
}