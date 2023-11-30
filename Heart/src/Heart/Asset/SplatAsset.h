#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    class SplatAsset : public Asset
    {
    public:
        SplatAsset(const HString8& path, const HString8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Splat; }

    protected:
        void LoadInternal() override;
        void UnloadInternal() override;

    private:

    };
}
