#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Renderer/Mesh.h"

namespace Heart
{
    class MeshAsset : public Asset
    {
    public:
        MeshAsset(const std::string& path)
            : Asset(path)
        { m_Type = Type::Mesh; }

        void Load() override;
        void Unload() override;

        Mesh& GetSubmesh(u32 index) { return m_Submeshes[index]; }
        u32 GetSubmeshCount() const { return static_cast<u32>(m_Submeshes.size()); }
        u32 GetMaxTextures() const { return static_cast<u32>(m_DefaultTexturePaths.size()); }
        const std::vector<std::string>& GetDefaultTexturePaths() const { return m_DefaultTexturePaths; }

    private:
        struct BufferView
        {
            BufferView(u32 index, u32 length, u32 offset)
                : BufferIndex(index), ByteLength(length), ByteOffset(offset)
            {}

            u32 BufferIndex;
            u32 ByteLength;
            u32 ByteOffset;
        };

        struct Accessor
        {
            Accessor(u32 index, u32 offset, u32 count, u32 type)
                : BufferViewIndex(index), ByteOffset(offset), Count(count), ComponentType(type)
            {}

            u32 BufferViewIndex;
            u32 ByteOffset;
            u32 Count;
            u32 ComponentType;
        };

        struct TextureView
        {
            TextureView(u32 sampIndex, u32 srcIndex)
                : SamplerIndex(sampIndex), SourceIndex(srcIndex)
            {}

            u32 SamplerIndex;
            u32 SourceIndex;
        };

        struct SubmeshParseData
        {
            u32 VertexOffset = 0;
            u32 IndexOffset = 0;
            std::vector<Mesh::Vertex> Vertices = {};
            std::vector<u32> Indices = {};
        };

    private:
        void ParseGLTF(unsigned char* data);
        std::vector<unsigned char> Base64Decode(const std::string& encoded);

        inline bool IsBase64(unsigned char c)
        {
            return (isalnum(c) || (c == '+') || (c == '/'));
        }

    private:
        static inline const std::string s_Base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::vector<Mesh> m_Submeshes;
        std::vector<std::string> m_DefaultTexturePaths;
    };
}