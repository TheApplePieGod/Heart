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
#include "Heart/Container/HVector.hpp"
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
            
            HVector<char> infoLog(logLength);
            glGetProgramInfoLog(m_ProgramId, logLength, &logLength, infoLog.Data());

            HE_ENGINE_LOG_ERROR("Failed to link OpenGL program: {0}", infoLog.Data());
            HE_ENGINE_ASSERT(false);
        }

        // generate timestamp queries
        if (m_Info.AllowPerformanceQuerying)
            for (size_t i = 0; i < m_QueryIds.size(); i++)
                glGenQueries(static_cast<int>(m_QueryIds[i].size()), m_QueryIds[i].data());
    }

    OpenGLComputePipeline::~OpenGLComputePipeline()
    {
        glDeleteProgram(m_ProgramId);
        if (m_Info.AllowPerformanceQuerying)
            for (size_t i = 0; i < m_QueryIds.size(); i++)
                glDeleteQueries(static_cast<int>(m_QueryIds[i].size()), m_QueryIds[i].data());
    }

    void OpenGLComputePipeline::Bind()
    {
        glUseProgram(m_ProgramId);
    }

    void OpenGLComputePipeline::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* _buffer)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(elementCount + elementOffset <= _buffer->GetAllocatedCount(), "ElementCount + ElementOffset must be <= buffer allocated count");
        HE_ENGINE_ASSERT(_buffer->GetType() == Buffer::Type::Uniform || _buffer->GetType() == Buffer::Type::Storage || _buffer->GetType() == Buffer::Type::Indirect, "Buffer bind must be either a uniform, storage, or indirect buffer");

        OpenGLBuffer& buffer = static_cast<OpenGLBuffer&>(*_buffer);

        int bufferType = OpenGLCommon::BufferTypeToOpenGL(buffer.GetType());
        if (buffer.GetType() == Buffer::Type::Indirect)
            bufferType = GL_SHADER_STORAGE_BUFFER; // indirect buffers need to be bound as storage buffers

        glBindBufferBase(bufferType, bindingIndex, buffer.GetBufferId());
        glBindBufferRange(bufferType, bindingIndex, buffer.GetBufferId(), elementOffset * buffer.GetLayout().GetStride(), elementCount * buffer.GetLayout().GetStride());
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

        u64 frameIndex = App::Get().GetFrameCount() % 2;

        int maxCountX, maxCountY, maxCountZ = 0;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxCountX);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxCountY);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxCountZ);

        if (m_Info.AllowPerformanceQuerying)
            glQueryCounter(m_QueryIds[frameIndex][0], GL_TIMESTAMP);

        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        glDispatchCompute(
            std::min(static_cast<u32>(maxCountX), m_DispatchCountX),
            std::min(static_cast<u32>(maxCountY), m_DispatchCountY),
            std::min(static_cast<u32>(maxCountZ), m_DispatchCountZ)
        );

        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        if (m_Info.AllowPerformanceQuerying)
        {
            glQueryCounter(m_QueryIds[frameIndex][1], GL_TIMESTAMP);

            u64 t1 = 0;
            u64 t2 = 0;
            glGetQueryObjectui64v(m_QueryIds[frameIndex][0], GL_QUERY_RESULT, &t1);
            glGetQueryObjectui64v(m_QueryIds[frameIndex][1], GL_QUERY_RESULT, &t2);
            m_PerformanceTimestamp = (t2 - t1) * 0.000001;
        }

        m_FlushedThisFrame = false;
    }
}