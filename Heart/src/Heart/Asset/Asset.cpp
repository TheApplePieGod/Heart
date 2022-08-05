#include "hepch.h"
#include "Asset.h"

#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/SceneAsset.h"

namespace Heart
{
    Asset::Asset(const HStringView& path, const HStringView& absolutePath)
        : m_Path(path), m_AbsolutePath(absolutePath)
    {
        UpdatePath(path, absolutePath);
    }

    void Asset::Reload()
    {
        if (m_Loaded)
            Unload();
        Load();
    }

    void Asset::UpdatePath(const HStringView& path, const HStringView& absolutePath)
    {
        auto entry = std::filesystem::path(path.DataUTF8());
        m_Filename = (HString)entry.filename().generic_u8string();
        m_Extension = (HString)entry.extension().generic_u8string();
        m_ParentPath = (HString)entry.parent_path().generic_u8string();

        // convert the extension to lowercase
        std::transform(m_Extension.BeginUTF8(), m_Extension.EndUTF8(), m_Extension.BeginUTF8(), [](unsigned char c) { return std::tolower(c); });

        m_Path = path;
        m_AbsolutePath = absolutePath;
    }

    Ref<Asset> Asset::Create(Type type, const HStringView& path, const HStringView& absolutePath)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create asset: selected type is not supported"); return nullptr; }
            case Asset::Type::Texture:
            { return CreateRef<TextureAsset>(path, absolutePath); }
            case Asset::Type::Shader:
            { return CreateRef<ShaderAsset>(path, absolutePath); }
            case Asset::Type::Mesh:
            { return CreateRef<MeshAsset>(path, absolutePath); }
            case Asset::Type::Material:
            { return CreateRef<MaterialAsset>(path, absolutePath); }
            case Asset::Type::Scene:
            { return CreateRef<SceneAsset>(path, absolutePath); }
        }
    }

    // adapted from https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
    std::vector<unsigned char> Asset::Base64Decode(const HStringView& encoded)
    {
        int in_len = static_cast<int>(encoded.GetCountUTF8());
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        while (in_len-- && (encoded.GetUTF8(in_) != '=') && IsBase64(encoded.GetUTF8(in_)))
        {
            char_array_4[i++] = encoded.GetUTF8(in_); in_++;
            if (i ==4)
            {
                for (i = 0; i <4; i++)
                    char_array_4[i] = static_cast<unsigned char>(s_Base64Chars.FindUTF8Char(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j <4; j++)
                char_array_4[j] = 0;

            for (j = 0; j <4; j++)
                char_array_4[j] = static_cast<unsigned char>(s_Base64Chars.FindUTF8Char(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                ret.push_back(char_array_3[j]);
        }

        return ret;
    }
}