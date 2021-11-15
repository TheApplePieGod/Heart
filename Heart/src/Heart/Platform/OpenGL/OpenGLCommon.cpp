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
            case ColorFormat::R32F: return GL_R32F;
            case ColorFormat::RG32F: return GL_RG32F;
            case ColorFormat::RGB32F: return GL_RGB32F;
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
            case ColorFormat::R32F: return GL_RED;
            case ColorFormat::RG32F: return GL_RG;
            case ColorFormat::RGB32F: return GL_RGB;
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
            case ColorFormat::R32F: return GL_FLOAT;
            case ColorFormat::RG32F: return GL_FLOAT;
            case ColorFormat::RGB32F: return GL_FLOAT;
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
        }

        return -1;
    }
}