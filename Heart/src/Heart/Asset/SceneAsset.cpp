#include "hepch.h"
#include "SceneAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace Heart
{
    void SceneAsset::Load()
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        try
        {
            m_Scene = DeserializeScene(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene at path {0}", m_AbsolutePath);
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        m_Data = nullptr;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void SceneAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Scene.reset();

        m_Data = nullptr;
        m_Loaded = false;
        m_Valid = false;
    }

    Ref<Scene> SceneAsset::DeserializeScene(const std::string& path)
    {
        auto scene = CreateRef<Scene>();

        u32 fileLength;
        unsigned char* data = FilesystemUtils::ReadFile(path, fileLength);
        if (!data)
            throw std::exception();

        auto j = nlohmann::json::parse(data);

        // parse entities
        {
            auto& field = j["entities"];
            for (auto& loaded : field)
            {
                // REQUIRED: Id & name components
                UUID id = static_cast<UUID>(loaded["idComponent"]["id"]);
                std::string name = loaded["nameComponent"]["name"];
                auto entity = scene->CreateEntityWithUUID(name, id);

                // REQUIRED: Transform component
                glm::vec3 translation = { loaded["transformComponent"]["translation"][0], loaded["transformComponent"]["translation"][1], loaded["transformComponent"]["translation"][2] };
                glm::vec3 rotation = { loaded["transformComponent"]["rotation"][0], loaded["transformComponent"]["rotation"][1], loaded["transformComponent"]["rotation"][2] };
                glm::vec3 scale = { loaded["transformComponent"]["scale"][0], loaded["transformComponent"]["scale"][1], loaded["transformComponent"]["scale"][2] };
                entity.SetTransform(translation, rotation, scale);

                // Parent component
                if (loaded.contains("parentComponent"))
                    entity.AddComponent<ParentComponent>(static_cast<UUID>(loaded["parentComponent"]["id"]));

                // Child component
                if (loaded.contains("childComponent"))
                {
                    auto& children = loaded["childComponent"]["children"];
                    std::vector<UUID> ids;
                    ids.reserve(children.size());
                    for (auto& childId : children)
                        ids.emplace_back(static_cast<UUID>(childId));
                    entity.AddComponent<ChildComponent>(ids);
                }

                // Mesh component
                if (loaded.contains("meshComponent"))
                {
                    auto& materials = loaded["meshComponent"]["materials"];
                    std::vector<UUID> ids;
                    UUID meshAsset = AssetManager::RegisterAsset(Asset::Type::Mesh, loaded["meshComponent"]["mesh"]["path"], false, loaded["meshComponent"]["mesh"]["engineResource"]);
                    for (auto& material : materials)
                        ids.emplace_back(AssetManager::RegisterAsset(Asset::Type::Material, material["path"], false, material["engineResource"]));
                    entity.AddComponent<MeshComponent>(meshAsset, ids);
                }
            }
        }

        // parse settings
        {
            auto& field = j["settings"];
            if (field.contains("environmentMap"))
                scene->SetEnvironmentMap(AssetManager::RegisterAsset(Asset::Type::Texture, field["environmentMap"]["path"], false, field["environmentMap"]["engineResource"]));
        }

        delete[] data;
        return scene;
    }

    void SceneAsset::SerializeScene(const std::string& path, Scene* scene)
    {
        nlohmann::json j;

        // entities
        {
            auto& field = j["entities"];
            
            u32 index = 0;
            scene->GetRegistry().each([scene, &index, &field](auto handle)
            {
                nlohmann::json entry;
                Entity entity = { scene, handle };
                
                // REQUIRED: Id component
                entry["idComponent"]["id"] = static_cast<u64>(entity.GetUUID());

                // REQUIRED: Name component
                entry["nameComponent"]["name"] = entity.GetName();

                // REQUIRED: Transform component
                auto& transformComp = entity.GetComponent<TransformComponent>();
                entry["transformComponent"]["translation"] = nlohmann::json::array({ transformComp.Translation.x, transformComp.Translation.y, transformComp.Translation.z });
                entry["transformComponent"]["rotation"] = nlohmann::json::array({ transformComp.Rotation.x, transformComp.Rotation.y, transformComp.Rotation.z });
                entry["transformComponent"]["scale"] = nlohmann::json::array({ transformComp.Scale.x, transformComp.Scale.y, transformComp.Scale.z });

                // Parent component
                if (entity.HasComponent<ParentComponent>())
                    entry["parentComponent"]["id"] = static_cast<u64>(entity.GetComponent<ParentComponent>().ParentUUID);

                // Child component
                if (entity.HasComponent<ChildComponent>())
                {
                    auto& childComp = entity.GetComponent<ChildComponent>();
                    for (size_t i = 0; i < childComp.Children.size(); i++)
                        entry["childComponent"]["children"][i] = static_cast<u64>(childComp.Children[i]);
                }

                // Mesh component
                if (entity.HasComponent<MeshComponent>())
                {
                    auto& meshComp = entity.GetComponent<MeshComponent>();
                    entry["meshComponent"]["mesh"]["path"] = AssetManager::GetPathFromUUID(meshComp.Mesh);
                    entry["meshComponent"]["mesh"]["engineResource"] = AssetManager::IsAssetAnEngineResource(meshComp.Mesh);
                    for (size_t i = 0; i < meshComp.Materials.size(); i++)
                    {
                        entry["meshComponent"]["materials"][i]["path"] = AssetManager::GetPathFromUUID(meshComp.Materials[i]);
                        entry["meshComponent"]["materials"][i]["engineResource"] = AssetManager::IsAssetAnEngineResource(meshComp.Materials[i]);
                    }
                }

                field[index++] = entry;
            });
        }

        // settings
        {
            auto& field = j["settings"];
            field["environmentMap"]["path"] = scene->GetEnvironmentMap() ? AssetManager::GetPathFromUUID(scene->GetEnvironmentMap()->GetMapAsset()) : "";
            field["environmentMap"]["engineResource"] = scene->GetEnvironmentMap() ? AssetManager::IsAssetAnEngineResource(scene->GetEnvironmentMap()->GetMapAsset()) : false;
        }

        std::ofstream file(path);
        file << j;
    }
}