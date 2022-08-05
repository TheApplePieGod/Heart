#pragma once

#include "Heart/Asset/Asset.h"
#include "Heart/Core/UUID.h"
#include "Heart/Renderer/Mesh.h"
#include "Heart/Renderer/Material.h"
#include "glm/mat4x4.hpp"
#include "nlohmann/json.hpp"

namespace Heart
{
    class MeshAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        MeshAsset(const HStringView& path, const HStringView& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Mesh; }

        void Load() override;
        void Unload() override;

        /**
         * @brief Get a submesh of the loaded mesh at the specified index.
         * 
         * @param index The index of the submesh.
         * @return A reference to the submesh. 
         */
        inline Mesh& GetSubmesh(u32 index) { return m_Submeshes[index]; }

        /*! @brief Get the total number of submeshes of the loaded mesh. */
        inline u32 GetSubmeshCount() const { return static_cast<u32>(m_Submeshes.size()); }

        /*! @brief Get the total number of material slots of the loaded mesh. */
        inline u32 GetMaxMaterials() const { return static_cast<u32>(m_DefaultMaterials.size()); }

        /*! @brief Get a reference to the default materials loaded with the mesh. */
        inline std::vector<Material>& GetDefaultMaterials() { return m_DefaultMaterials; }

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

        struct TextureSource
        {
            TextureSource(const HStringView& path, UUID assetId)
                : Path(path), AssetId(assetId)
            {}

            HString Path;
            UUID AssetId;
        };

        struct PrimitiveParseData
        {
            std::vector<Mesh::Vertex> Vertices = {};
            std::vector<u32> Indices = {};
            u32 MaterialIndex = 0;
        };

        struct MeshParseData
        {
            std::vector<PrimitiveParseData> Primitives;
        };

        struct SubmeshData
        {
            std::vector<Mesh::Vertex> Vertices = {};
            std::vector<u32> Indices = {};
            u32 VertexOffset;
            u32 IndexOffset;
        };

    private:
        void ParseGLTF(unsigned char* data);
        void ParseGLTFNode(const nlohmann::json& root, u32 nodeIndex, const std::vector<MeshParseData>& meshData, std::unordered_map<u32, SubmeshData>& submeshData, const glm::mat4& parentTransform);

    private:
        std::vector<Mesh> m_Submeshes;
        std::vector<Material> m_DefaultMaterials;
    };
}