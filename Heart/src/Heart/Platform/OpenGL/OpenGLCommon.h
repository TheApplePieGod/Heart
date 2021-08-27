#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/Framebuffer.h"

namespace Heart
{
    struct OpenGLCommon
    {
        static int ShaderTypeToOpenGL(Shader::Type type);
        static int BufferDataTypeToBaseOpenGL(BufferDataType type);
        static int VertexTopologyToOpenGL(VertexTopology topology);
        static int CullModeToOpenGL(CullMode mode);
        static int MsaaSampleCountToOpenGL(MsaaSampleCount sampleCount);
    };
}