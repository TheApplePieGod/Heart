#include "hepch.h"
#include "OpenGLComputePipeline.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"
#include "Heart/Platform/OpenGL/OpenGLShader.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Renderer/Renderer.h"
#include "glad/glad.h"

namespace Heart
{
    OpenGLComputePipeline::OpenGLComputePipeline(const ComputePipelineCreateInfo& createInfo)
        : ComputePipeline(createInfo)
    {
        // create the shader program
        m_ProgramId = glCreateProgram();

        auto compShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.ComputeShaderAsset)->GetShader();
        glAttachShader(m_ProgramId, static_cast<OpenGLShader*>(compShader)->GetShaderId());

        glLinkProgram(m_ProgramId);

        int success;
        glGetProgramiv(m_ProgramId, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLint logLength = 0;
            glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &logLength);
            
            std::vector<char> infoLog(logLength);
            glGetProgramInfoLog(m_ProgramId, logLength, &logLength, infoLog.data());

            HE_ENGINE_LOG_ERROR("Failed to link OpenGL program: {0}", infoLog.data());
            HE_ENGINE_ASSERT(false);
        }
    }

    OpenGLComputePipeline::~OpenGLComputePipeline()
    {
        glDeleteProgram(m_ProgramId);
    }

    void OpenGLComputePipeline::Bind()
    {
        glUseProgram(m_ProgramId);
    }

    void OpenGLComputePipeline::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* _buffer)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(elementCount + elementOffset <= _buffer->GetAllocatedCount(), "ElementCount + ElementOffset must be <= buffer allocated count");
        HE_ENGINE_ASSERT(_buffer->GetType() == Buffer::Type::Uniform || _buffer->GetType() == Buffer::Type::Storage, "Buffer bind must be either a uniform or storage buffer");

        OpenGLBuffer& buffer = static_cast<OpenGLBuffer&>(*_buffer);

        glBindBufferBase(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId());
        glBindBufferRange(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId(), elementOffset * buffer.GetLayout().GetStride(), elementCount * buffer.GetLayout().GetStride());
    }

    void OpenGLComputePipeline::BindShaderTextureResource(u32 bindingIndex, Texture* _texture)
    {
        HE_PROFILE_FUNCTION();

        OpenGLTexture& texture = static_cast<OpenGLTexture&>(*_texture);

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        glBindTexture(texture.GetTarget(), texture.GetTextureId());
    }

    void OpenGLComputePipeline::BindShaderTextureLayerResource(u32 bindingIndex, Texture* _texture, u32 layerIndex, u32 mipLevel)
    {
        HE_PROFILE_FUNCTION();

        OpenGLTexture& texture = static_cast<OpenGLTexture&>(*_texture);

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        glBindTexture(GL_TEXTURE_2D, texture.GetLayerTextureId(layerIndex, mipLevel));
    }

    void OpenGLComputePipeline::FlushBindings()
    {
        m_FlushedThisFrame = true;
    }

    void OpenGLComputePipeline::Submit()
    {
        HE_PROFILE_FUNCTION();

        int maxCountX, maxCountY, maxCountZ = 0;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxCountX);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxCountY);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxCountZ);

        glDispatchCompute(
            std::min(static_cast<u32>(maxCountX), m_DispatchCountX),
            std::min(static_cast<u32>(maxCountY), m_DispatchCountY),
            std::min(static_cast<u32>(maxCountZ), m_DispatchCountZ)
        );

        m_FlushedThisFrame = false;
    }
}