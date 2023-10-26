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
        ShaderAsset(const HString8& path, const HString8& absolutePath);

        /*! @brief Get the shader stored in this asset. */
        inline Ref<Flourish::Shader> GetShader() { return m_Shader; }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;

    private:
        Ref<Flourish::Shader> m_Shader;
        Flourish::ShaderType m_ShaderType = Flourish::ShaderTypeFlags::None;
    };
}
