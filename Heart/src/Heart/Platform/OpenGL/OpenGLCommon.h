#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Renderer/Framebuffer.h"

namespace Heart
{
    struct OpenGLCommon
    {
        static int ColorFormatToInternalOpenGL(ColorFormat format);
        static int ColorFormatToOpenGL(ColorFormat format);
        static int ColorFormatToOpenGLDataType(ColorFormat format);
        static int ShaderTypeToOpenGL(Shader::Type type);
        static int BufferDataTypeToBaseOpenGL(BufferDataType type);
        static int VertexTopologyToOpenGL(VertexTopology topology);
        static int CullModeToOpenGL(CullMode mode);
        static int WindingOrderToOpenGL(WindingOrder order);
        static int MsaaSampleCountToOpenGL(MsaaSampleCount sampleCount);
        static int ShaderResourceTypeToOpenGL(ShaderResourceType type);
        static int BufferTypeToOpenGL(Buffer::Type type);
        static int BufferUsageTypeToOpenGL(BufferUsageType type);
        static int BlendFactorToOpenGL(BlendFactor factor);
        static int BlendOperationToOpenGL(BlendOperation op);
        static int SamplerFilterToOpenGL(SamplerFilter filter);
        static int SamplerFilterToOpenGLWithMipmap(SamplerFilter filter);
        static int SamplerWrapModeToOpenGL(SamplerWrapMode mode);\
        static int SamplerReductionModeToOpenGL(SamplerReductionMode mode);
    };
}