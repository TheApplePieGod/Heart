#include "htpch.h"
#include "OpenGLCommon.h"

#include "glad/glad.h"

namespace Heart
{
    int OpenGLCommon::ColorFormatToInternalOpenGL(ColorFormat format)
    {
        switch (format)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified ColorFormat"); } break;
            case ColorFormat::RGBA8: return GL_RGBA8;
            case ColorFormat::R16F: return GL_R16F;
            case ColorFormat::RGBA16F: return GL_RGBA16F;
            case ColorFormat::R32F: return GL_R32F;
            case ColorFormat::RGBA32F: return GL_RGBA32F;
        }

        return -1;
    }

    int OpenGLCommon::ColorFormatToOpenGL(ColorFormat format)
    {
        switch (format)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified ColorFormat"); } break;
            case ColorFormat::RGBA8: return GL_RGBA;
            case ColorFormat::R16F: return GL_RED;
            case ColorFormat::RGBA16F: return GL_RGBA;
            case ColorFormat::R32F: return GL_RED;
            case ColorFormat::RGBA32F: return GL_RGBA;
        }

        return -1;
    }

    int OpenGLCommon::ColorFormatToOpenGLDataType(ColorFormat format)
    {
        switch (format)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified ColorFormat"); } break;
            case ColorFormat::RGBA8: return GL_UNSIGNED_BYTE;
            case ColorFormat::R16F: return GL_FLOAT;
            case ColorFormat::RGBA16F: return GL_FLOAT;
            case ColorFormat::R32F: return GL_FLOAT;
            case ColorFormat::RGBA32F: return GL_FLOAT;
        }

        return -1;
    }

    int OpenGLCommon::ShaderTypeToOpenGL(Shader::Type type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified ShaderType"); } break;
            case Shader::Type::Vertex : return GL_VERTEX_SHADER;
            case Shader::Type::Fragment: return GL_FRAGMENT_SHADER;
        }

        return -1;
    }

    int OpenGLCommon::BufferDataTypeToBaseOpenGL(BufferDataType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified BufferDataType"); } break;
            case BufferDataType::Bool: return GL_BOOL;
            case BufferDataType::UInt: return GL_UNSIGNED_INT;
            case BufferDataType::Double: return GL_DOUBLE;
            case BufferDataType::Int: return GL_INT;
            case BufferDataType::Int2: return GL_INT;
            case BufferDataType::Int3: return GL_INT;
            case BufferDataType::Int4: return GL_INT;
            case BufferDataType::Float: return GL_FLOAT;
            case BufferDataType::Float2: return GL_FLOAT;
            case BufferDataType::Float3: return GL_FLOAT;
            case BufferDataType::Float4: return GL_FLOAT;
            //case BufferDataType::Mat3: return VK_FORMAT_UNDEFINED;
            //case BufferDataType::Mat4: return VK_FORMAT_UNDEFINED;
        }

        return -1;
    }

    int OpenGLCommon::VertexTopologyToOpenGL(VertexTopology topology)
    {
        switch (topology)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified VertexTopology"); } break;
            case VertexTopology::TriangleList: return GL_TRIANGLES;
            case VertexTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
            case VertexTopology::TriangleFan: return GL_TRIANGLE_FAN;
            case VertexTopology::PointList: return GL_POINTS;
            case VertexTopology::LineList: return GL_LINES;
            case VertexTopology::LineStrip: return GL_LINE_STRIP;
        }

        return -1;
    }

    int OpenGLCommon::CullModeToOpenGL(CullMode mode)
    {
        switch (mode)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified CullMode"); } break;
            case CullMode::None: return GL_NONE;
            case CullMode::Backface: return GL_BACK;
            case CullMode::Frontface: return GL_FRONT;
            case CullMode::BackAndFront: return GL_FRONT_AND_BACK;
        }

        return -1;
    }

    int OpenGLCommon::WindingOrderToOpenGL(WindingOrder order)
    {
        // they are flipped in opengl for some reason (likely due to the coordinate system)
        switch (order)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified WindingOrder"); } break;
            case WindingOrder::Clockwise: return GL_CCW;
            case WindingOrder::CounterClockwise: return GL_CW;
        }

        return -1;
    }

    int OpenGLCommon::MsaaSampleCountToOpenGL(MsaaSampleCount sampleCount)
    {
        switch (sampleCount)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified MsaaSampleCount"); } break;
            case MsaaSampleCount::None: return 1;
            case MsaaSampleCount::Two: return 2;
            case MsaaSampleCount::Four: return 4;
            case MsaaSampleCount::Eight: return 8;
            case MsaaSampleCount::Sixteen: return 16;
            case MsaaSampleCount::Thirtytwo: return 32;
            case MsaaSampleCount::Sixtyfour: return 64;
        }

        return -1;
    }

    int OpenGLCommon::ShaderResourceTypeToOpenGL(ShaderResourceType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified ShaderResourceType"); } break;
            case ShaderResourceType::Texture : return GL_TEXTURE;
            case ShaderResourceType::UniformBuffer: return GL_UNIFORM_BUFFER;
            case ShaderResourceType::StorageBuffer: return GL_SHADER_STORAGE_BUFFER;
        }

        return -1;
    }

    int OpenGLCommon::BufferTypeToOpenGL(Buffer::Type type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified BufferType"); } break;
            case Buffer::Type::Vertex : return GL_ARRAY_BUFFER;
            case Buffer::Type::Index: return GL_ELEMENT_ARRAY_BUFFER;
            case Buffer::Type::Uniform: return GL_UNIFORM_BUFFER;
            case Buffer::Type::Storage: return GL_SHADER_STORAGE_BUFFER;
            case Buffer::Type::Pixel: return GL_PIXEL_PACK_BUFFER;
            case Buffer::Type::Indirect: return GL_DRAW_INDIRECT_BUFFER;
        }

        return -1;
    }

    int OpenGLCommon::BufferUsageTypeToOpenGL(BufferUsageType type)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified BufferUsageType"); } break;
            case BufferUsageType::Static : return GL_STATIC_DRAW;
            case BufferUsageType::Dynamic: return GL_DYNAMIC_DRAW;
        }

        return -1;
    }

    int OpenGLCommon::BlendFactorToOpenGL(BlendFactor factor)
    {
        switch (factor)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified BlendFactor"); } break;
            case BlendFactor::Zero: return GL_ZERO;
            case BlendFactor::One: return GL_ONE;

            case BlendFactor::SrcColor: return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor: return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;

            case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
        }

        return -1;
    }

    int OpenGLCommon::BlendOperationToOpenGL(BlendOperation op)
    {
        switch (op)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified BlendOperation"); } break;
            case BlendOperation::Add: return GL_FUNC_ADD;
            case BlendOperation::Subtract: return GL_FUNC_SUBTRACT;
            case BlendOperation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
            case BlendOperation::Min: return GL_MIN;
            case BlendOperation::Max: return GL_MAX;
        }

        return -1;
    }

    int OpenGLCommon::SamplerFilterToOpenGL(SamplerFilter filter)
    {
        switch (filter)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified SamplerFilter"); } break;
            case SamplerFilter::Linear: return GL_LINEAR;
            case SamplerFilter::Nearest: return GL_NEAREST;
        }

        return -1;
    }

    int OpenGLCommon::SamplerFilterToOpenGLWithMipmap(SamplerFilter filter)
    {
        switch (filter)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified SamplerFilter"); } break;
            case SamplerFilter::Linear: return GL_LINEAR_MIPMAP_LINEAR;
            case SamplerFilter::Nearest: return GL_NEAREST_MIPMAP_NEAREST;
        }

        return -1;
    }

    int OpenGLCommon::SamplerWrapModeToOpenGL(SamplerWrapMode mode)
    {
        switch (mode)
        {
            default:
            { HE_ENGINE_ASSERT(false, "OpenGL does not support specified SamplerWrapMode"); } break;
            case SamplerWrapMode::ClampToBorder: return GL_CLAMP_TO_BORDER;
            case SamplerWrapMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
            case SamplerWrapMode::Repeat: return GL_REPEAT;
            case SamplerWrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
        }

        return -1;
    }
}