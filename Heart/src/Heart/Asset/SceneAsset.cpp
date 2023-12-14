#include "hepch.h"
#include "SceneAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Scripting/ScriptClass.h"
#include "Heart/Scripting/ScriptingEngine.h"

namespace Heart
{
    void SceneAsset::LoadInternal()
    {
        HE_PROFILE_FUNCTION();

        try
        {
            m_Scene = DeserializeScene(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene at path {0}", m_AbsolutePath.Data());
            return;
        }

        m_Data = nullptr;
        m_Valid = true;
    }

    void SceneAsset::UnloadInternal()
    {
        m_Scene.reset();
        m_Data = nullptr;
    }

    void SceneAsset::Save(Scene* scene)
    {
        SerializeScene(m_AbsolutePath, scene);
        if (m_Loaded)
            m_Scene = scene->Clone();
    }

    Ref<Scene> SceneAsset::DeserializeScene(const HStringView8& path)
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
                HString8 name = loaded["nameComponent"]["name"];
                auto entity = scene->CreateEntityWithUUID(name, id);

                // Parent component
                if (loaded.contains("parentComponent"))
                    entity.AddComponent<ParentComponent>(static_cast<UUID>(loaded["parentComponent"]["id"]));

                // Child component
                if (loaded.contains("childrenComponent"))
                {
                    auto& children = loaded["childrenComponent"]["children"];
                    HVector<UUID> ids;
                    ids.Reserve(children.size());
                    for (auto& childId : children)
                        ids.AddInPlace(static_cast<UUID>(childId));
                    entity.AddComponent<ChildrenComponent>(ids);
                }

                // Mesh component
                if (loaded.contains("meshComponent"))
                {
                    auto& compEntry = loaded["meshComponent"];
                    auto& materials = compEntry["materials"];
                    HVector<UUID> materialIds;
                    UUID meshAsset = AssetManager::RegisterAsset(Asset::Type::Mesh, compEntry["mesh"]["path"], false, compEntry["mesh"]["engineResource"]);
                    for (auto& material : materials)
                        materialIds.AddInPlace(AssetManager::RegisterAsset(Asset::Type::Material, material["path"], false, material["engineResource"]));
                    entity.AddComponent<MeshComponent>(meshAsset, materialIds);
                }

                // Splat component
                if (loaded.contains("splatComponent"))
                {
                    auto& compEntry = loaded["splatComponent"];
                    UUID splatAsset = AssetManager::RegisterAsset(Asset::Type::Splat, compEntry["splat"]["path"], false, compEntry["splat"]["engineResource"]);
                    entity.AddComponent<SplatComponent>(splatAsset);
                }

                // Light component
                if (loaded.contains("lightComponent"))
                {
                    auto& compEntry = loaded["lightComponent"];
                    LightComponent comp;
                    comp.Color = { compEntry["color"][0], compEntry["color"][1], compEntry["color"][2], compEntry["color"][3] };
                    comp.LightType = compEntry["lightType"];
                    if (compEntry.contains("radius"))
                        comp.Radius = compEntry["radius"];
                    entity.AddComponent<LightComponent>(comp);
                }

                // Camera component
                if (loaded.contains("cameraComponent"))
                {
                    auto& compEntry = loaded["cameraComponent"];
                    CameraComponent comp;
                    comp.FOV = compEntry["fov"];
                    comp.NearClipPlane = compEntry["nearClip"];
                    comp.FarClipPlane = compEntry["farClip"];
                    entity.AddComponent<CameraComponent>(comp);
                    if (compEntry["primary"])
                        entity.AddComponent<PrimaryCameraComponent>();
                }
                
                // Rigid body component
                if (loaded.contains("collisionComponent"))
                {
                    auto& compEntry = loaded["collisionComponent"];
                    CollisionComponent comp;
                    PhysicsBody body;
                    PhysicsBodyType bodyType = compEntry["type"];
                    PhysicsBodyShape shapeType = compEntry["shape"];
                    PhysicsBodyCreateInfo bodyInfo;
                    bodyInfo.Type = bodyType;
                    bodyInfo.ExtraData = (void*)(intptr_t)id;
                    bodyInfo.Mass = compEntry["mass"];
                    if (compEntry.contains("collisionChannels"))
                        bodyInfo.CollisionChannels = compEntry["collisionChannels"];
                    if (compEntry.contains("collisionMask"))
                        bodyInfo.CollisionMask = compEntry["collisionMask"];
                    switch (shapeType)
                    {
                        default:
                        { HE_ENGINE_ASSERT(false, "Unsupported shape type"); }

                        case PhysicsBodyShape::Box:
                        {
                            glm::vec3 extent = {
                                compEntry["extent"][0],
                                compEntry["extent"][1],
                                compEntry["extent"][2]
                            };
                            body = PhysicsBody::CreateBoxShape(bodyInfo, extent);
                        } break;

                        case PhysicsBodyShape::Sphere:
                        {
                            float radius = compEntry["radius"];
                            body = PhysicsBody::CreateSphereShape(bodyInfo, radius);
                        } break;
                            
                        case PhysicsBodyShape::Capsule:
                        {
                            float radius = compEntry["radius"];
                            float height = compEntry["height"];
                            body = PhysicsBody::CreateCapsuleShape(bodyInfo, radius, height);
                        } break;
                    }
                    
                    comp.BodyId = scene->GetPhysicsWorld().AddBody(body);

                    entity.AddComponent<CollisionComponent>(comp);
                }

                // Text component
                if (loaded.contains("textComponent"))
                {
                    auto& compEntry = loaded["textComponent"];
                    TextComponent comp;
                    comp.Font = AssetManager::RegisterAsset(
                        Asset::Type::Font,
                        compEntry["font"]["path"],
                        false,
                        compEntry["font"]["engineResource"]
                    );
                    if (compEntry.contains("material"))
                    {
                        comp.Material = AssetManager::RegisterAsset(
                            Asset::Type::Material,
                            compEntry["material"]["path"],
                            false,
                            compEntry["material"]["engineResource"]
                        );
                    }
                    comp.Text = compEntry["text"];
                    comp.FontSize = compEntry["fontSize"];
                    comp.LineHeight = compEntry["lineHeight"];
                    
                    entity.AddComponent<TextComponent>(comp);
                }

                // Runtime components
                if (loaded.contains("runtimeComponents"))
                {
                    auto& compEntry = loaded["runtimeComponents"];
                    for (auto& j : compEntry)
                    {
                        ScriptComponentInstance instance;
                        HString typeName = j["type"];
                        s64 typeId = ScriptingEngine::GetClassIdFromName(typeName);
                        instance.SetScriptClassId(typeId);
                        if (!instance.IsInstantiable())
                        {
                            HE_ENGINE_LOG_WARN(
                                "Component class '{0}' referenced in entity is no longer instantiable (id: {1}, name: {2})",
                                typeName.DataUTF8(),id, entity.GetName().DataUTF8()
                            );
                        }
                        else
                        {
                            instance.Instantiate();
                            instance.LoadFieldsFromJson(j["fields"]);
                            entity.AddRuntimeComponent(typeId, instance.GetObjectHandle());
                        }
                    }
                }
            }

            // Load transform & script components once all entities have been parsed since their
            // initialization may depend on other entities
            for (auto& loaded : field)
            {
                UUID id = static_cast<UUID>(loaded["idComponent"]["id"]);
                Entity entity = scene->GetEntityFromUUID(id);
                
                if (!entity.IsValid()) continue;
                
                {
                    auto& compEntry = loaded["transformComponent"];
                    glm::vec3 translation = { compEntry["translation"][0], compEntry["translation"][1], compEntry["translation"][2] };
                    glm::vec3 rotation = { compEntry["rotation"][0], compEntry["rotation"][1], compEntry["rotation"][2] };
                    glm::vec3 scale = { compEntry["scale"][0], compEntry["scale"][1], compEntry["scale"][2] };
                    entity.SetTransform(translation, rotation, scale, false);
                }
                    
                // Script component
                if (loaded.contains("scriptComponent"))
                {
                    auto& compEntry = loaded["scriptComponent"];
                    ScriptComponent comp;
                    if (compEntry.contains("type"))
                    {
                        HString typeName = compEntry["type"];
                        s64 typeId = ScriptingEngine::GetClassIdFromName(typeName);
                        comp.Instance.SetScriptClassId(typeId);
                        if (!comp.Instance.IsInstantiable())
                        {
                            HE_ENGINE_LOG_WARN(
                                "Script class '{0}' referenced in entity is no longer instantiable (id: {1}, name: {2})",
                                typeName.DataUTF8(),id, entity.GetName().DataUTF8()
                            );
                        }
                        else
                        {
                            comp.Instance.Instantiate(entity);
                            comp.Instance.LoadFieldsFromJson(compEntry["fields"]);
                            comp.Instance.OnConstruct();
                        }
                    }
                    entity.AddComponent<ScriptComponent>(comp);
                }
            }
        }

        // Cache initial transforms once everything has been instantiated and initialized
        scene->CacheDirtyTransforms();

        // parse settings
        {
            auto& field = j["settings"];
            if (field.contains("environmentMap"))
                scene->SetEnvironmentMap(AssetManager::RegisterAsset(Asset::Type::Texture, field["environmentMap"]["path"], false, field["environmentMap"]["engineResource"]));
            if (field.contains("physics"))
            {
                scene->GetPhysicsWorld().SetGravity({
                    field["physics"]["gravity"][0],
                    field["physics"]["gravity"][1],
                    field["physics"]["gravity"][2]
                });
            }
        }

        delete[] data;
        return scene;
    }

    void SceneAsset::SerializeScene(const HStringView8& path, Scene* scene)
    {
        nlohmann::json j;

        // entities
        {
            auto& field = j["entities"];
            
            u32 index = 0;
            for (auto [handle] : scene->GetRegistry().storage<entt::entity>().each())
            {
                nlohmann::json entry;
                Entity entity = { scene, handle };
                
                // REQUIRED: Id component
                entry["idComponent"]["id"] = static_cast<u64>(entity.GetUUID());

                // REQUIRED: Name component
                entry["nameComponent"]["name"] = entity.GetName();

                // REQUIRED: Transform component
                {
                    auto& compEntry = entry["transformComponent"];
                    auto& comp = entity.GetComponent<TransformComponent>();
                    compEntry["translation"] = nlohmann::json::array({ comp.Translation.x, comp.Translation.y, comp.Translation.z });
                    compEntry["rotation"] = nlohmann::json::array({ comp.Rotation.x, comp.Rotation.y, comp.Rotation.z });
                    compEntry["scale"] = nlohmann::json::array({ comp.Scale.x, comp.Scale.y, comp.Scale.z });
                }

                // Parent component
                if (entity.HasComponent<ParentComponent>())
                    entry["parentComponent"]["id"] = static_cast<u64>(entity.GetComponent<ParentComponent>().ParentUUID);

                // Child component
                if (entity.HasComponent<ChildrenComponent>())
                {
                    auto& compEntry = entry["childrenComponent"]["children"];
                    auto& comp = entity.GetComponent<ChildrenComponent>();
                    for (size_t i = 0; i < comp.Children.Count(); i++)
                        compEntry[i] = static_cast<u64>(comp.Children[i]);
                }

                // Mesh component
                if (entity.HasComponent<MeshComponent>())
                {
                    auto& compEntry = entry["meshComponent"];
                    auto& comp = entity.GetComponent<MeshComponent>();
                    compEntry["mesh"]["path"] = AssetManager::GetPathFromUUID(comp.Mesh);
                    compEntry["mesh"]["engineResource"] = AssetManager::IsAssetAResource(comp.Mesh);
                    for (size_t i = 0; i < comp.Materials.Count(); i++)
                    {
                        compEntry["materials"][i]["path"] = AssetManager::GetPathFromUUID(comp.Materials[i]);
                        compEntry["materials"][i]["engineResource"] = AssetManager::IsAssetAResource(comp.Materials[i]);
                    }
                }

                // Splat component
                if (entity.HasComponent<SplatComponent>())
                {
                    auto& compEntry = entry["splatComponent"];
                    auto& comp = entity.GetComponent<SplatComponent>();
                    compEntry["splat"]["path"] = AssetManager::GetPathFromUUID(comp.Splat);
                    compEntry["splat"]["engineResource"] = AssetManager::IsAssetAResource(comp.Splat);
                }

                // Light component
                if (entity.HasComponent<LightComponent>())
                {
                    auto& compEntry = entry["lightComponent"];
                    auto& comp = entity.GetComponent<LightComponent>();
                    compEntry["color"] = nlohmann::json::array({ comp.Color.r, comp.Color.g, comp.Color.b, comp.Color.a });
                    compEntry["lightType"] = comp.LightType;
                    compEntry["radius"] = comp.Radius;
                }

                // Script component
                if (entity.HasComponent<ScriptComponent>())
                {
                    auto& compEntry = entry["scriptComponent"];
                    auto& comp = entity.GetComponent<ScriptComponent>();
                    if (comp.Instance.HasScriptClass())
                    {
                        compEntry["type"] = comp.Instance.GetScriptClassObject().GetFullName();
                        compEntry["fields"] = comp.Instance.SerializeFieldsToJson();
                    }
                }

                // Camera component
                if (entity.HasComponent<CameraComponent>())
                {
                    auto& compEntry = entry["cameraComponent"];
                    auto& comp = entity.GetComponent<CameraComponent>();
                    compEntry["primary"] = entity.HasComponent<PrimaryCameraComponent>();
                    compEntry["fov"] = comp.FOV;
                    compEntry["nearClip"] = comp.NearClipPlane;
                    compEntry["farClip"] = comp.FarClipPlane;
                }
                
                // Rigid body component
                if (entity.HasComponent<CollisionComponent>())
                {
                    auto& compEntry = entry["collisionComponent"];
                    auto& comp = entity.GetComponent<CollisionComponent>();
                    PhysicsBody* body = scene->GetPhysicsWorld().GetBody(comp.BodyId);
                    compEntry["type"] = body->GetBodyType();
                    compEntry["shape"] = body->GetShapeType();
                    compEntry["mass"] = body->GetMass();
                    compEntry["collisionChannels"] = body->GetCollisionChannels();
                    compEntry["collisionMask"] = body->GetCollisionMask();
                    switch (body->GetShapeType())
                    {
                        default:
                        { HE_ENGINE_ASSERT(false, "Unsupported body type"); }

                        case PhysicsBodyShape::Box:
                        {
                            auto extent = body->GetBoxExtent();
                            compEntry["extent"] = nlohmann::json::array({ extent.x, extent.y, extent.z });
                        } break;

                        case PhysicsBodyShape::Sphere:
                        {
                            compEntry["radius"] = body->GetSphereRadius();
                        } break;
                            
                        case PhysicsBodyShape::Capsule:
                        {
                            compEntry["radius"] = body->GetCapsuleRadius();
                            compEntry["height"] = body->GetCapsuleHeight();
                        } break;
                    }
                }
                
                // Text component
                if (entity.HasComponent<TextComponent>())
                {
                    auto& compEntry = entry["textComponent"];
                    auto& comp = entity.GetComponent<TextComponent>();
                    compEntry["font"]["path"] = AssetManager::GetPathFromUUID(comp.Font);
                    compEntry["font"]["engineResource"] = AssetManager::IsAssetAResource(comp.Font);
                    compEntry["material"]["path"] = AssetManager::GetPathFromUUID(comp.Material);
                    compEntry["material"]["engineResource"] = AssetManager::IsAssetAResource(comp.Material);
                    compEntry["text"] = comp.Text;
                    compEntry["fontSize"] = comp.FontSize;
                    compEntry["lineHeight"] = comp.LineHeight;
                }

                // Runtime components
                {
                    nlohmann::json j;
                    HVector<nlohmann::json> runComps;
                    for (auto& pair : Heart::ScriptingEngine::GetComponentClasses())
                    {
                        if (!entity.HasRuntimeComponent(pair.first)) continue;

                        auto& comp = entity.GetRuntimeComponent(pair.first);
                        if (comp.Instance.HasScriptClass())
                        {
                            j["type"] = comp.Instance.GetScriptClassObject().GetFullName();
                            j["fields"] = comp.Instance.SerializeFieldsToJson();
                            runComps.AddInPlace(j);
                        }
                    }
                    if (runComps.Count() > 0)
                        entry["runtimeComponents"] = runComps;
                }

                field[index++] = entry;
            }
        }

        // settings
        {
            auto& field = j["settings"];
            
            // env map
            {
                field["environmentMap"]["path"] = scene->GetEnvironmentMap() ? AssetManager::GetPathFromUUID(scene->GetEnvironmentMap()->GetMapAsset()) : "";
                field["environmentMap"]["engineResource"] = scene->GetEnvironmentMap() ? AssetManager::IsAssetAResource(scene->GetEnvironmentMap()->GetMapAsset()) : false;
            }
            
            // physics
            {
                auto grav = scene->GetPhysicsWorld().GetGravity();
                field["physics"]["gravity"] = nlohmann::json::array({ grav.x, grav.y, grav.z });
            }
        }

        FilesystemUtils::WriteFile(path, j);
    }
}
