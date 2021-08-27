#include "htpch.h"
#include "OpenGLCommon.h"

#include "glad/glad.h"

namespace Heart
{
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
            case VertexTopology::PointList: return GL_POINTS;
            case VertexTopology::LineList: return GL_LINES;
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
            case CullMode::Both: return GL_FRONT_AND_BACK;
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
}