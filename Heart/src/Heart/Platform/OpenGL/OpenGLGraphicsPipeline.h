#pragma once

#include "Heart/Renderer/Pipeline.h"

namespace Heart
{
    class OpenGLGraphicsPipeline : public GraphicsPipeline
    {
    public:
        OpenGLGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);
        ~OpenGLGraphicsPipeline() override;

        inline u32 GetProgramId() const { return m_ProgramId; }
        inline u32 GetVertexArrayId() const { return m_VertexArrayId; }

    private:
        u32 m_ProgramId;
        u32 m_VertexArrayId;
    };
}