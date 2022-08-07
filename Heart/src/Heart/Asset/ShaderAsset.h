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
        ShaderAsset(const HStringView8& path, const HStringView8& absolutePath);

        void Load(bool async = false) override;
        void Unload() override;

        /*! @brief Get a pointer to the shader stored in this asset. */
        inline Shader* GetShader() { return m_Shader.get(); }

    private:
        Ref<Shader> m_Shader;
        Shader::Type m_ShaderType = Shader::Type::None;
    };
}