#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "Flourish/Api/Buffer.h"

namespace Heart
{
    class SplatAsset : public Asset
    {
    public:
        SplatAsset(const HString8& path, const HString8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Splat; }

        inline const Flourish::Buffer* GetTransformBuffer() const { return m_TransformBuffer.get(); }
        inline const Flourish::Buffer* GetColorBuffer() const { return m_ColorBuffer.get(); }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;

    private:
        void ParseSplat(unsigned char* data, u32 length);

    private:
        Ref<Flourish::Buffer> m_TransformBuffer;
        Ref<Flourish::Buffer> m_ColorBuffer;
    };
}
