#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "Flourish/Api/Buffer.h"
#include "glm/glm.hpp"

namespace Heart
{
    class SplatAsset : public Asset
    {
    public:
        SplatAsset(const HString8& path, const HString8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Splat; }

        inline const Flourish::Buffer* GetDataBuffer() const { return m_DataBuffer.get(); }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;
        bool ShouldUnload() override;

    private:
        struct SplatData
        {
            glm::uvec4 PackedSigma;
            glm::vec4 Position;
            glm::vec4 Color;
        };

    private:
        void ParseSplat(unsigned char* data, u32 length);

    private:
        Ref<Flourish::Buffer> m_DataBuffer;
    };
}
