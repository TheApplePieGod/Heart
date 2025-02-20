#include "hepch.h"
#include "ShaderAsset.h"

namespace Heart
{
    ShaderAsset::ShaderAsset(const HString8& path, const HString8& absolutePath)
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
        else if (m_Extension == ".rint")
            m_ShaderType = Flourish::ShaderTypeFlags::RayIntersection;
        else if (m_Extension == ".rahit")
            m_ShaderType = Flourish::ShaderTypeFlags::RayAnyHit;
    }

    void ShaderAsset::LoadInternal()
    {
        HE_PROFILE_FUNCTION();

        Flourish::ShaderCreateInfo createInfo;
        createInfo.Type = m_ShaderType;
        createInfo.Path = std::string_view(m_AbsolutePath.Data(), m_AbsolutePath.Count());
        m_Shader = Flourish::Shader::Create(createInfo);

        m_Data = nullptr;
        m_Valid = true;
    }

    void ShaderAsset::UnloadInternal()
    {
        m_Shader.reset();
        //delete[] m_Data;
        m_Data = nullptr;
    }

    bool ShaderAsset::ShouldUnload()
    {
        // This is the only remaining reference
        return m_Shader.use_count() == 1;
    }
}
