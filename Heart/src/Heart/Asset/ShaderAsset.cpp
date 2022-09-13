#include "hepch.h"
#include "ShaderAsset.h"

namespace Heart
{
    ShaderAsset::ShaderAsset(const HStringView8& path, const HStringView8& absolutePath)
        : Asset(path, absolutePath)
    {
        m_Type = Type::Shader;
        
        // infer the shader type based on the file extension
        if (m_Extension == ".vert")
            m_ShaderType = Shader::Type::Vertex;
        else if (m_Extension == ".frag")
            m_ShaderType = Shader::Type::Fragment;
        else if (m_Extension == ".comp")
            m_ShaderType = Shader::Type::Compute;
    }

    void ShaderAsset::Load(bool async)
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