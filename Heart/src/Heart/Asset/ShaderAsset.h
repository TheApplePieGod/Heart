#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Shader.h"

namespace Heart
{
    class ShaderAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        ShaderAsset(const std::string& path, const std::string& absolutePath);

        void Load() override;
        void Unload() override;

        /*! @brief Get a pointer to the shader stored in this asset. */
        Shader* GetShader() { return m_Shader.get(); }

    private:
        Ref<Shader> m_Shader;
        Shader::Type m_ShaderType = Shader::Type::None;
    };
}