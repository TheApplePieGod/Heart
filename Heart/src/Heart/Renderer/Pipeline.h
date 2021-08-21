#pragma once

#include "Heart/Renderer/Shader.h"

namespace Heart
{
    enum class VertexTopology
    {
        TriangleList = 0, PointList = 1, LineList = 2
    };

    struct GraphicsPipelineCreateInfo
    {
        Ref<Shader> VertexShader;
        Ref<Shader> FragmentShader;

        VertexTopology VertexTopology;

        bool DepthEnable;
    };

    struct ComputePipelineCreateInfo
    {
        
    };

    class Pipeline
    {
    public:
        enum class Type
        {
            None = 0, Graphics = 1, Compute = 2
        };

    public:
        virtual ~Pipeline() = default;
    };

    class GraphicsPipeline : public Pipeline
    {
    public:
        GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~GraphicsPipeline() = default;

    private:
        GraphicsPipelineCreateInfo m_Info;
    };

    class ComputePipeline : public Pipeline
    {
    public:
        ComputePipeline(const ComputePipelineCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~ComputePipeline() = default;

    public:
        static Ref<ComputePipeline> Create(const ComputePipelineCreateInfo& createInfo);

    private:
        ComputePipelineCreateInfo m_Info;
    };
}