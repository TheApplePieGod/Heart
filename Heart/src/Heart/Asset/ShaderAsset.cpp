#include "htpch.h"
#include "ShaderAsset.h"

namespace Heart
{
    ShaderAsset::ShaderAsset(const std::string& path)
        : Asset(path)
    {
        m_Type = Type::Shader;
        
        // infer the shader type based on the file extension
        if (m_Extension == ".vert")
            m_ShaderType = Shader::Type::Vertex;
        else if (m_Extension == ".frag")
            m_ShaderType = Shader::Type::Fragment;
    }

    void ShaderAsset::Load()
    {
        if (m_Loaded) return;

        m_Shader = Shader::Create(m_Path, m_ShaderType);

        m_Data = nullptr;
        m_Loaded = true;
    }

    void ShaderAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Shader.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Loaded = false;
    }
}