#pragma once

namespace Heart
{
    struct GraphicsPipelineCreateInfo
    {
        std::string m_VertexShaderName;
        std::string m_FragmentShaderName;
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

    public:
        static Ref<GraphicsPipeline> Create(const GraphicsPipelineCreateInfo& createInfo);

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

    class PipelineRegistry
    {
    public:
        Ref<Pipeline> RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo);
        Ref<Pipeline> RegisterComputePipeline(const std::string& name, const ComputePipelineCreateInfo& createInfo);
        Ref<Pipeline> LoadPipeline(const std::string& name);

    private:
        std::unordered_map<std::string, Ref<Pipeline>> m_Pipelines;
    };
}