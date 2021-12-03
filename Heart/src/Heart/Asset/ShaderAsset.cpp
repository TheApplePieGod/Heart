#include "hepch.h"
#include "ShaderAsset.h"

namespace Heart
{
    ShaderAsset::ShaderAsset(const std::string& path, const std::string& absolutePath)
        : Asset(path, absolutePath)
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
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        m_Shader = Shader::Create(m_AbsolutePath, m_ShaderType);

        m_Data = nullptr;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void ShaderAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Shader.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Loaded = false;
        m_Valid = false;
    }
}