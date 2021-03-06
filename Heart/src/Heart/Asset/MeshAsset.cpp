#include "hepch.h"
#include "MeshAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Renderer/Texture.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Heart
{
    void MeshAsset::Load()
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        u32 fileLength;
        unsigned char* data = nullptr;

        try
        {
            data = FilesystemUtils::ReadFile(m_AbsolutePath, fileLength);
            if (!data)
                throw std::exception();
            if (/*m_Extension == ".glb" ||*/ m_Extension == ".gltf") // TODO: glb support
                ParseGLTF(data);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load mesh at path {0}", m_AbsolutePath);
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        delete[] data;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void MeshAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Submeshes.clear();
        m_DefaultMaterials.clear();
        m_Loaded = false;
        m_Valid = false;
    }

    void MeshAsset::ParseGLTF(unsigned char* data)
    {
        auto j = nlohmann::json::parse(data);
        
        // parse buffers
        std::vector<std::vector<unsigned char>> buffers;
        for (auto& buffer : j["buffers"])
        {
            std::string uri = buffer["uri"];
            if (uri.find("base64") != std::string::npos)
            {
                std::string base64 = uri.substr(uri.find(',') + 1);
                buffers.emplace_back(Base64Decode(base64));
            }
            else if (uri.find(".bin") != std::string::npos)
            {
                u32 fileLength;
                std::string binPath = std::filesystem::path(m_AbsolutePath).parent_path().append(uri).generic_u8string();
                unsigned char* bin = FilesystemUtils::ReadFile(binPath, fileLength);
                buffers.emplace_back();
                buffers.back().assign(bin, bin + fileLength);
            }
            else
            {
                HE_ENGINE_LOG_ERROR("Cannot load GLTF mesh that uses an unsupported buffer data format");
                throw std::exception();
            }
        }

        // parse buffer views
        std::vector<BufferView> bufferViews;
        for (auto& view : j["bufferViews"])
        {
            u32 byteOffset = 0;
            if (view.contains("byteOffset"))
                byteOffset = view["byteOffset"];
            bufferViews.emplace_back(view["buffer"], view["byteLength"], byteOffset);
        }

        // parse accessors
        std::vector<Accessor> accessors;
        for (auto& accessor : j["accessors"])
        {
            u32 byteOffset = 0;
            if (accessor.contains("byteOffset"))
                byteOffset = accessor["byteOffset"];
            accessors.emplace_back(accessor["bufferView"], byteOffset, accessor["count"], accessor["componentType"]);
        }

        // parse texture sources
        std::vector<TextureSource> textureSources;
        if (j.contains("images"))
        {
            for (auto& image : j["images"])
            {
                std::string uri = image["uri"];
                if (uri.find("base64") != std::string::npos)
                {
                    HE_ENGINE_LOG_ERROR("Cannot load GLTF mesh that uses inline textures (not supported yet)");
                    throw std::exception();
                    //std::string base64 = uri.substr(uri.find(',') + 1);
                    //buffers.emplace_back(Base64Decode(base64));
                }
                else
                {
                    std::string finalPath = std::filesystem::path(m_ParentPath).append(uri).generic_u8string();
                    finalPath = std::regex_replace(finalPath, std::regex("%20"), " "); // replace URL encoded spaces with actual spaces
                    textureSources.emplace_back(finalPath, AssetManager::RegisterAsset(Asset::Type::Texture, finalPath));
                }
            }
        }

        // parse textures
        std::vector<TextureView> textureViews;
        if (j.contains("textures"))
        {
            for (auto& texture : j["textures"])
            {
                u32 samplerIndex = 0;
                if (texture.contains("sampler"))
                    samplerIndex = texture["sampler"];
                textureViews.emplace_back(samplerIndex, texture["source"]);
            }
        }

        // parse materials
        std::string materialFilenameStart = "material";
        std::string materialFilenameEnd = ".hemat";
        if (j.contains("materials"))
        {
            u32 materialIndex = 0;
            for (auto& material : j["materials"])
            {
                auto& pbrField = material["pbrMetallicRoughness"];
                Material parsingMaterial;

                if (pbrField.contains("baseColorTexture"))
                {
                    u32 texIndex = pbrField["baseColorTexture"]["index"];
                    parsingMaterial.m_AlbedoTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;

                    // in case this field is not defined, we default the factor to 1.f to make sure the texture gets utilized
                    parsingMaterial.m_MaterialData.SetBaseColor({ 1.f, 1.f, 1.f, 1.f });

                    // TODO: change this possibly
                    auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(parsingMaterial.m_AlbedoTextureAsset);
                    if (texAsset && texAsset->IsValid())
                    {
                        if (texAsset->GetTexture()->HasTransparency())
                            parsingMaterial.m_MaterialData.SetAlphaClipThreshold(0.1f);
                        parsingMaterial.m_Translucent = texAsset->GetTexture()->HasTranslucency();
                    }
                }
                if (pbrField.contains("baseColorFactor"))
                    parsingMaterial.m_MaterialData.SetBaseColor({ pbrField["baseColorFactor"][0], pbrField["baseColorFactor"][1], pbrField["baseColorFactor"][2], pbrField["baseColorFactor"][3] });

                if (pbrField.contains("metallicRoughnessTexture"))
                {
                    u32 texIndex = pbrField["metallicRoughnessTexture"]["index"];
                    parsingMaterial.m_MetallicRoughnessTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;

                    // in case these fields are not defined, we default the factor to 1.f to make sure the texture gets utilized
                    parsingMaterial.m_MaterialData.SetMetalnessFactor(1.f);
                    parsingMaterial.m_MaterialData.SetRoughnessFactor(1.f);
                }
                if (pbrField.contains("metallicFactor"))
                    parsingMaterial.m_MaterialData.SetMetalnessFactor(pbrField["metallicFactor"]);
                if (pbrField.contains("roughnessFactor"))
                    parsingMaterial.m_MaterialData.SetRoughnessFactor(pbrField["roughnessFactor"]);

                if (material.contains("normalTexture"))
                {
                    u32 texIndex = material["normalTexture"]["index"];
                    parsingMaterial.m_NormalTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;

                    if (material["normalTexture"].contains("scale"))
                    {
                        float scale = material["normalTexture"]["scale"];
                        parsingMaterial.m_MaterialData.SetTexCoordScale({ scale, scale });
                    }
                }
                if (material.contains("occlusionTexture"))
                {
                    u32 texIndex = material["occlusionTexture"]["index"];
                    parsingMaterial.m_OcclusionTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;
                }
                if (material.contains("emissiveTexture"))
                {
                    u32 texIndex = material["emissiveTexture"]["index"];
                    parsingMaterial.m_EmissiveTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;

                    // in case this field is not defined, we default the factor to 1.f to make sure the texture gets utilized
                    parsingMaterial.m_MaterialData.SetEmissiveFactor({ 1.f, 1.f, 1.f, 1.f });
                }
                if (material.contains("emissiveFactor"))
                    parsingMaterial.m_MaterialData.SetEmissiveFactor({ material["emissiveFactor"][0], material["emissiveFactor"][1], material["emissiveFactor"][2], 0.f });

                m_DefaultMaterials.emplace_back(parsingMaterial);
                materialIndex++;
            }
        }
        else // should always have one material
            m_DefaultMaterials.emplace_back();

        // parse meshes
        bool hasTangents = false;
        bool hasNormals = false;
        std::vector<MeshParseData> meshes;
        for (auto& mesh : j["meshes"])
        {
            MeshParseData meshData;
            for (auto& primitive : mesh["primitives"])
            {
                PrimitiveParseData parseData;

                u32 indexOffset = 0;
                u32 vertexOffset = 0;
                u32 materialIndex = 0;
                if (primitive.contains("material"))
                    materialIndex = primitive["material"];
                parseData.MaterialIndex = materialIndex;

                // Points = 0, Lines, LineLoop, LineStrip, Triangles, TriangleStrip, TriangleFan
                if (primitive.contains("mode") && primitive["mode"] != 4)
                {
                    HE_ENGINE_LOG_ERROR("Cannot load GLTF mesh that uses an unsupported primitive mode");
                    throw std::exception();
                }
                if (!primitive.contains("indices"))
                {
                    HE_ENGINE_LOG_ERROR("Cannot load GLTF mesh that does not use indexed geometry");
                    throw std::exception();
                }

                // parse indices
                {
                    auto& accessor = accessors[primitive["indices"]];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count > parseData.Indices.size())
                        parseData.Indices.resize(accessor.Count);

                    u32 offset = 0; // bytes
                    for (int i = static_cast<int>(accessor.Count) - 1; i >= 0; i--) // parse the indices in reverse order to account for coordinate system flip
                    {
                        if (accessor.ComponentType == 5123) // unsigned short
                        {
                            parseData.Indices[i] = *(u16*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 2;
                        }
                        else if (accessor.ComponentType == 5125) // unsigned int
                        {
                            parseData.Indices[i] = *(u32*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 4;
                        }
                        else
                        {
                            HE_ENGINE_LOG_ERROR("Cannot load GLTF mesh that has indices with an unsupported data type");
                            throw std::exception();
                        }
                    }
                }

                u32 lastCount = 0;
                for (auto& attribute : primitive["attributes"].items())
                {
                    auto& accessor = accessors[attribute.value()];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count > parseData.Vertices.size())
                        parseData.Vertices.resize(accessor.Count);

                    // parse vertex data
                    u32 offset = 0; // bytes
                    for (u32 i = 0; i < accessor.Count; i++)
                    {
                        if (attribute.key() == "POSITION")
                        {
                            parseData.Vertices[i].Position = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 12;
                        }
                        else if (attribute.key() == "TEXCOORD_0")
                        {
                            parseData.Vertices[i].UV = *(glm::vec2*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 8;
                        }
                        else if (attribute.key() == "NORMAL")
                        {
                            parseData.Vertices[i].Normal = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            hasNormals = true;
                            offset += 12;
                        }
                        else if (attribute.key() == "TANGENT")
                        {
                            parseData.Vertices[i].Tangent = *(glm::vec4*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            hasTangents = true;
                            offset += 16;
                        }
                    }

                    lastCount = accessor.Count;
                }

                meshData.Primitives.push_back(parseData);
            }

            meshes.push_back(meshData);
        }

        // iterate through the nodes and create submeshes based on each material in the scene
        std::unordered_map<u32, SubmeshData> submeshData;
        u32 sceneIndex = j["scene"];
        auto& scenes = j["scenes"];
        
        // start with a base scale of z = -1. this is because we are translating from a right handed to a left handed coordinate space
        // the order of the indices have been reversed to account for this change
        glm::mat4 baseTransform = glm::scale(glm::mat4(1.f), { 1.f, 1.f, -1.f });
        for (auto& nodeIndex : scenes[sceneIndex]["nodes"])
        {
            ParseGLTFNode(j, nodeIndex, meshes, submeshData, baseTransform);
        }

        // populate submeshes with parsed data
        for (auto& pair : submeshData)
        {
            // calculate the normals and/or tangents of the submesh if they aren't provided
            if (!hasNormals || !hasTangents)
            {
                for (size_t i = 0; i < pair.second.Indices.size(); i += 3)
                {
                    Mesh::Vertex& v0 = pair.second.Vertices[pair.second.Indices[i]];
                    Mesh::Vertex& v1 = pair.second.Vertices[pair.second.Indices[i + 1]];
                    Mesh::Vertex& v2 = pair.second.Vertices[pair.second.Indices[i + 2]];

                    glm::vec3 deltaPos1 = v1.Position - v0.Position;
                    glm::vec3 deltaPos2 = v2.Position - v0.Position;

                    glm::vec2 deltaUV1 = v1.UV - v0.UV;
                    glm::vec2 deltaUV2 = v2.UV - v0.UV;

                    float r = 1.0f / (deltaUV1.y * deltaUV2.x - deltaUV1.x * deltaUV2.y);
                    glm::vec4 tangent = glm::vec4((deltaPos1 * -deltaUV2.y + deltaPos2 * deltaUV1.y) * r, 1.f);

                    if (!hasTangents)
                    {
                        v0.Tangent = tangent;
                        v1.Tangent = tangent;
                        v2.Tangent = tangent;
                    }
                    if (!hasNormals)
                    {
                        glm::vec3 bitangent = (deltaPos1 * -deltaUV2.x + deltaPos2 * deltaUV1.x) * r;
                        glm::vec3 normal = glm::cross(bitangent, glm::vec3(tangent));

                        v0.Normal = normal;
                        v1.Normal = normal;
                        v2.Normal = normal;
                    }
                }
            }

            m_Submeshes.emplace_back(pair.second.Vertices, pair.second.Indices, pair.first);
        }
    }

    void MeshAsset::ParseGLTFNode(const nlohmann::json& root, u32 nodeIndex, const std::vector<MeshParseData>& meshData, std::unordered_map<u32, SubmeshData>& submeshData, const glm::mat4& parentTransform)
    {
        auto& node = root["nodes"][nodeIndex];

        glm::mat4 positionMatrix;
        if (node.contains("matrix"))
        {
            std::vector<float> valueArr(16);
            for (size_t i = 0; i < node["matrix"].size(); i++)
                valueArr[i] = node["matrix"][i];
            positionMatrix = glm::make_mat4(valueArr.data());
        }
        else
        {
            glm::vec3 translation = { 0.f, 0.f, 0.f };
            glm::quat rotation = { 1.f, 0.f, 0.f, 0.f };
            glm::vec3 scale = { 1.f, 1.f, 1.f };

            if (node.contains("translation"))
                translation = { node["translation"][0], node["translation"][1], node["translation"][2] };
            if (node.contains("rotation"))
                rotation = { node["rotation"][3], node["rotation"][0], node["rotation"][1], node["rotation"][2] };
            if (node.contains("scale"))
                scale = { node["scale"][0], node["scale"][1], node["scale"][2] };

            positionMatrix = glm::translate(glm::mat4(1.0f), translation)
				* glm::toMat4(rotation)
				* glm::scale(glm::mat4(1.0f), scale);
        }

        glm::mat4 finalMatrix = parentTransform * positionMatrix;

        if (node.contains("mesh"))
        {
            auto& mesh = meshData[node["mesh"]];

            for (auto& primitive : mesh.Primitives)
            {
                if (submeshData.find(primitive.MaterialIndex) == submeshData.end())
                    submeshData[primitive.MaterialIndex] = SubmeshData();
                auto& submesh = submeshData[primitive.MaterialIndex];

                // push the new indices into the material submesh
                for (auto index : primitive.Indices)
                    submesh.Indices.emplace_back(index + submesh.VertexOffset);

                // transform the vertices and push them into the material submesh
                for (auto vertex : primitive.Vertices)
                {
                    vertex.Position = finalMatrix * glm::vec4(vertex.Position, 1.f);
                    vertex.Normal = (glm::mat3)finalMatrix * vertex.Normal;
                    vertex.Tangent = glm::vec4((glm::mat3)finalMatrix * glm::vec3(vertex.Tangent), vertex.Tangent.w);
                    submesh.Vertices.emplace_back(vertex);
                }

                submesh.IndexOffset += static_cast<u32>(primitive.Indices.size());
                submesh.VertexOffset += static_cast<u32>(primitive.Vertices.size());
            }
        }

        if (node.contains("children"))
            for (auto& child : node["children"])
                ParseGLTFNode(root, child, meshData, submeshData, finalMatrix);
    }
}