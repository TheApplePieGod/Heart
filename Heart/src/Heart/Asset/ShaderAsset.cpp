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
            m_ShaderType = Flourish::ShaderTypeFlags::Vertex;
        else if (m_Extension == ".frag")
            m_ShaderType = Flourish::ShaderTypeFlags::Fragment;
        else if (m_Extension == ".comp")
            m_ShaderType = Flourish::ShaderTypeFlags::Compute;
        else if (m_Extension == ".rgen")
            m_ShaderType = Flourish::ShaderTypeFlags::RayGen;
        else if (m_Extension == ".rmiss")
            m_ShaderType = Flourish::ShaderTypeFlags::RayMiss;
        else if (m_Extension == ".rchit")
            m_ShaderType = Flourish::ShaderTypeFlags::RayClosestHit;
    }

    void ShaderAsset::Load(bool async)
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        Flourish::ShaderCreateInfo createInfo;
        createInfo.Type = m_ShaderType;
        createInfo.Path = std::string_view(m_AbsolutePath.Data(), m_AbsolutePath.Count());
        m_Shader = Flourish::Shader::Create(createInfo);

        m_Data = nullptr;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void ShaderAsset::Unload()
    {
        if (!m_Loaded) return;
        m_Loaded = false;

        m_Shader.reset();
        //delete[] m_Data;
        m_Data = nullptr;
        m_Valid = false;
    }
}
