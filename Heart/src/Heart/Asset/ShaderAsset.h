#pragma once

#include "Heart/Asset/Asset.h"
#include "Flourish/Api/Shader.h"

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

        /*! @brief Get the shader stored in this asset. */
        inline Ref<Flourish::Shader> GetShader() { return m_Shader; }

    private:
        Ref<Flourish::Shader> m_Shader;
        Flourish::ShaderType m_ShaderType = Flourish::ShaderType::None;
    };
}