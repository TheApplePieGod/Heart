#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Shader.h"

namespace Heart
{
    class ShaderAsset : public Asset
    {
    public:
        ShaderAsset(const std::string& path);

        void Load() override;
        void Unload() override;

        Shader* GetShader() { return m_Shader.get(); }

    private:
        Ref<Shader> m_Shader;
        Shader::Type m_ShaderType = Shader::Type::None;
    };
}