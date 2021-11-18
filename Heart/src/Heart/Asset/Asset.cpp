#include "htpch.h"
#include "Asset.h"

#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"

namespace Heart
{
    Asset::Asset(const std::string& path, const std::string& absolutePath)
        : m_Path(path), m_AbsolutePath(absolutePath)
    {
        auto entry = std::filesystem::path(path);
        m_Filename = entry.filename().generic_u8string();
        m_Extension = entry.extension().generic_u8string();

        // convert the extension to lowercase
        std::transform(m_Extension.begin(), m_Extension.end(), m_Extension.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    void Asset::Reload()
    {
        if (m_Loaded)
            Unload();
        Load();
    }

    Ref<Asset> Asset::Create(Type type, const std::string& path, const std::string& absolutePath)
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
        }
    }

    // adapted from https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
    std::vector<unsigned char> Asset::Base64Decode(const std::string& encoded)
    {
        int in_len = static_cast<int>(encoded.size());
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        while (in_len-- && ( encoded[in_] != '=') && IsBase64(encoded[in_]))
        {
            char_array_4[i++] = encoded[in_]; in_++;
            if (i ==4)
            {
                for (i = 0; i <4; i++)
                    char_array_4[i] = static_cast<unsigned char>(s_Base64Chars.find(char_array_4[i]));

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
                char_array_4[j] = static_cast<unsigned char>(s_Base64Chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                ret.push_back(char_array_3[j]);
        }

        return ret;
    }
}