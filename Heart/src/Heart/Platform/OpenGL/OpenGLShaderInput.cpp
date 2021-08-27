#include "htpch.h"
#include "OpenGLShaderInput.h"

#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "Heart/Core/App.h"

namespace Heart
{
    OpenGLShaderInputSet::OpenGLShaderInputSet(std::initializer_list<ShaderInputElement> elements)
        : ShaderInputSet(elements)
    {

    }

    OpenGLShaderInputSet::~OpenGLShaderInputSet()
    {

    }

    ShaderInputBindPoint OpenGLShaderInputSet::CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements)
    {
        if (App::Get().GetFrameCount() != m_LastResetFrame)
        {
            m_BindDataPool.clear();
            m_LastResetFrame = App::Get().GetFrameCount();
        }

        HE_ENGINE_ASSERT(bindElements.size() <= 100, "Too many elements in OpenGL bind point");

        BindData& bindData = m_BindDataPool.emplace_back();
        u32 bufferIndex = 0;
        u32 imageIndex = 0;
        for (auto& element : bindElements)
        {
            if (element.TargetBuffer != nullptr)
            {
                OpenGLBuffer& buffer = static_cast<OpenGLBuffer&>(*element.TargetBuffer.get());

                bindData.Buffers[bufferIndex] = &buffer;

                bufferIndex++;
            }
            else if (element.TargetTexture != nullptr)
            {
                OpenGLTexture& texture = static_cast<OpenGLTexture&>(*element.TargetTexture.get());

                bindData.Textures[imageIndex] = &texture;

                imageIndex++;
            }
        }
        
        ShaderInputBindPoint bindPoint = {
            &bindData, bufferIndex, imageIndex
        };

        return bindPoint;
    }
}