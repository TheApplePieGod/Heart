#include "htpch.h"
#include "OpenGLGraphicsPipeline.h"

#include "glad/glad.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Platform/OpenGL/OpenGLShader.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"

namespace Heart
{
    OpenGLGraphicsPipeline::OpenGLGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
        : GraphicsPipeline(createInfo)
    {
        // create the shader program
        m_ProgramId = glCreateProgram();

        auto vertShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.VertexShaderAsset)->GetShader();
        auto fragShader = AssetManager::RetrieveAsset<ShaderAsset>(createInfo.FragmentShaderAsset)->GetShader();
        glAttachShader(m_ProgramId, static_cast<OpenGLShader*>(vertShader)->GetShaderId());
        glAttachShader(m_ProgramId, static_cast<OpenGLShader*>(fragShader)->GetShaderId());

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

        // setup vertex layout (VAO)
        glGenVertexArrays(1, &m_VertexArrayId);
        glBindVertexArray(m_VertexArrayId);

        u32 index = 0;
        for (auto& element : createInfo.VertexLayout.GetElements())
        {
            glEnableVertexAttribArray(index);
            //glVertexAttribPointer(index, BufferDataTypeComponents(element.DataType), OpenGLCommon::BufferDataTypeToBaseOpenGL(element.DataType), GL_FALSE, createInfo.VertexLayout.GetStride(), (void*)element.Offset);
            glVertexAttribFormat(index, BufferDataTypeComponents(element.DataType),  OpenGLCommon::BufferDataTypeToBaseOpenGL(element.DataType), GL_FALSE, element.Offset);
            glVertexAttribBinding(index, 0);
            index++;
        }
    }

    OpenGLGraphicsPipeline::~OpenGLGraphicsPipeline()
    {
        glDeleteProgram(m_ProgramId);
    }
}