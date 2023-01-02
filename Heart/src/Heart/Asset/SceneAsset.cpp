#include "hepch.h"
#include "SceneAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"
#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace Heart
{
    void SceneAsset::Load(bool async)
    {
        HE_PROFILE_FUNCTION();
        
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        try
        {
            m_Scene = DeserializeScene(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene at path {0}", m_AbsolutePath.Data());
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
        m_Loaded = false;

        m_Scene.reset();

        m_Data = nullptr;
        m_Valid = false;
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
                    auto& materials = loaded["meshComponent"]["materials"];
                    HVector<UUID> materialIds;
                    UUID meshAsset = AssetManager::RegisterAsset(Asset::Type::Mesh, loaded["meshComponent"]["mesh"]["path"], false, loaded["meshComponent"]["mesh"]["engineResource"]);
                    for (auto& material : materials)
                        materialIds.AddInPlace(AssetManager::RegisterAsset(Asset::Type::Material, material["path"], false, material["engineResource"]));
                    entity.AddComponent<MeshComponent>(meshAsset, materialIds);
                }

                // Light component
                if (loaded.contains("lightComponent"))
                {
                    LightComponent comp;
                    comp.Color = { loaded["lightComponent"]["color"][0], loaded["lightComponent"]["color"][1], loaded["lightComponent"]["color"][2], loaded["lightComponent"]["color"][3] };
                    comp.LightType = loaded["lightComponent"]["lightType"];
                    comp.ConstantAttenuation = loaded["lightComponent"]["attenuation"]["constant"];
                    comp.LinearAttenuation = loaded["lightComponent"]["attenuation"]["linear"];
                    comp.QuadraticAttenuation = loaded["lightComponent"]["attenuation"]["quadratic"];
                    entity.AddComponent<LightComponent>(comp);
                }

                // Camera component
                if (loaded.contains("cameraComponent"))
                {
                    CameraComponent comp;
                    comp.FOV = loaded["cameraComponent"]["fov"];
                    comp.NearClipPlane = loaded["cameraComponent"]["nearClip"];
                    comp.FarClipPlane = loaded["cameraComponent"]["farClip"];
                    entity.AddComponent<CameraComponent>(comp);
                    if (loaded["cameraComponent"]["primary"])
                        entity.AddComponent<PrimaryCameraComponent>();
                }
                
                // Rigid body component
                if (loaded.contains("collisionComponent"))
                {
                    CollisionComponent comp;
                    PhysicsBody body;
                    PhysicsBodyType bodyType = loaded["collisionComponent"]["type"];
                    PhysicsBodyShape shapeType = loaded["collisionComponent"]["shape"];
                    PhysicsBodyCreateInfo bodyInfo;
                    bodyInfo.Type = bodyType;
                    bodyInfo.ExtraData = (void*)(intptr_t)id;
                    bodyInfo.Mass = loaded["collisionComponent"]["mass"];
                    if (loaded["collisionComponent"].contains("collisionChannels"))
                        bodyInfo.CollisionChannels = loaded["collisionComponent"]["collisionChannels"];
                    if (loaded["collisionComponent"].contains("collisionMask"))
                        bodyInfo.CollisionMask = loaded["collisionComponent"]["collisionMask"];
                    switch (shapeType)
                    {
                        default:
                        { HE_ENGINE_ASSERT(false, "Unsupported shape type"); }

                        case PhysicsBodyShape::Box:
                        {
                            glm::vec3 extent = {
                                loaded["collisionComponent"]["extent"][0],
                                loaded["collisionComponent"]["extent"][1],
                                loaded["collisionComponent"]["extent"][2]
                            };
                            body = PhysicsBody::CreateBoxShape(bodyInfo, extent);
                        } break;

                        case PhysicsBodyShape::Sphere:
                        {
                            float radius = loaded["collisionComponent"]["radius"];
                            body = PhysicsBody::CreateSphereShape(bodyInfo, radius);
                        } break;
                            
                        case PhysicsBodyShape::Capsule:
                        {
                            float radius = loaded["collisionComponent"]["radius"];
                            float height = loaded["collisionComponent"]["height"];
                            body = PhysicsBody::CreateCapsuleShape(bodyInfo, radius, height);
                        } break;
                    }
                    
                    comp.BodyId = scene->GetPhysicsWorld().AddBody(body);

                    entity.AddComponent<CollisionComponent>(comp);
                }

            }

            // Load transform & script components once all entities have been parsed since their
            // initialization may depend on other entities
            for (auto& loaded : field)
            {
                UUID id = static_cast<UUID>(loaded["idComponent"]["id"]);
                Entity entity = scene->GetEntityFromUUID(id);
                
                if (!entity.IsValid()) continue;
                
                glm::vec3 translation = { loaded["transformComponent"]["translation"][0], loaded["transformComponent"]["translation"][1], loaded["transformComponent"]["translation"][2] };
                glm::vec3 rotation = { loaded["transformComponent"]["rotation"][0], loaded["transformComponent"]["rotation"][1], loaded["transformComponent"]["rotation"][2] };
                glm::vec3 scale = { loaded["transformComponent"]["scale"][0], loaded["transformComponent"]["scale"][1], loaded["transformComponent"]["scale"][2] };
                entity.SetTransform(translation, rotation, scale);
                
                // Script component
                if (loaded.contains("scriptComponent"))
                {
                    ScriptComponent comp;
                    HString8 scriptClass = loaded["scriptComponent"]["type"];
                    comp.Instance = ScriptInstance(scriptClass.ToHString());
                    if (comp.Instance.IsInstantiable())
                    {
                        if (!comp.Instance.ValidateClass())
                        {
                            HE_ENGINE_LOG_WARN(
                                "Class '{0}' referenced in scene is no longer instantiable",
                                scriptClass.Data()
                            );
                        }
                        else
                        {
                            comp.Instance.Instantiate(entity);
                            comp.Instance.LoadFieldsFromJson(loaded["scriptComponent"]["fields"]);
                            comp.Instance.OnConstruct();
                        }
                    }
                    entity.AddComponent<ScriptComponent>(comp);
                }
            }
        }

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
                if (entity.HasComponent<ChildrenComponent>())
                {
                    auto& childComp = entity.GetComponent<ChildrenComponent>();
                    for (size_t i = 0; i < childComp.Children.Count(); i++)
                        entry["childrenComponent"]["children"][i] = static_cast<u64>(childComp.Children[i]);
                }

                // Mesh component
                if (entity.HasComponent<MeshComponent>())
                {
                    auto& meshComp = entity.GetComponent<MeshComponent>();
                    entry["meshComponent"]["mesh"]["path"] = AssetManager::GetPathFromUUID(meshComp.Mesh);
                    entry["meshComponent"]["mesh"]["engineResource"] = AssetManager::IsAssetAResource(meshComp.Mesh);
                    for (size_t i = 0; i < meshComp.Materials.Count(); i++)
                    {
                        entry["meshComponent"]["materials"][i]["path"] = AssetManager::GetPathFromUUID(meshComp.Materials[i]);
                        entry["meshComponent"]["materials"][i]["engineResource"] = AssetManager::IsAssetAResource(meshComp.Materials[i]);
                    }
                }

                // Light component
                if (entity.HasComponent<LightComponent>())
                {
                    auto& lightComp = entity.GetComponent<LightComponent>();
                    entry["lightComponent"]["color"] = nlohmann::json::array({ lightComp.Color.r, lightComp.Color.g, lightComp.Color.b, lightComp.Color.a });
                    entry["lightComponent"]["lightType"] = lightComp.LightType;
                    entry["lightComponent"]["attenuation"]["constant"] = lightComp.ConstantAttenuation;
                    entry["lightComponent"]["attenuation"]["linear"] = lightComp.LinearAttenuation;
                    entry["lightComponent"]["attenuation"]["quadratic"] = lightComp.QuadraticAttenuation;
                }

                // Script component
                if (entity.HasComponent<ScriptComponent>())
                {
                    auto& scriptComp = entity.GetComponent<ScriptComponent>();
                    entry["scriptComponent"]["type"] = scriptComp.Instance.GetScriptClass();
                    entry["scriptComponent"]["fields"] = scriptComp.Instance.SerializeFieldsToJson();
                }

                // Camera component
                if (entity.HasComponent<CameraComponent>())
                {
                    auto& camComp = entity.GetComponent<CameraComponent>();
                    entry["cameraComponent"]["primary"] = entity.HasComponent<PrimaryCameraComponent>();
                    entry["cameraComponent"]["fov"] = camComp.FOV;
                    entry["cameraComponent"]["nearClip"] = camComp.NearClipPlane;
                    entry["cameraComponent"]["farClip"] = camComp.FarClipPlane;
                }
                
                // Rigid body component
                if (entity.HasComponent<CollisionComponent>())
                {
                    auto& bodyComp = entity.GetComponent<CollisionComponent>();
                    PhysicsBody* body = scene->GetPhysicsWorld().GetBody(bodyComp.BodyId);
                    entry["collisionComponent"]["type"] = body->GetBodyType();
                    entry["collisionComponent"]["shape"] = body->GetShapeType();
                    entry["collisionComponent"]["mass"] = body->GetMass();
                    entry["collisionComponent"]["collisionChannels"] = body->GetCollisionChannels();
                    entry["collisionComponent"]["collisionMask"] = body->GetCollisionMask();
                    switch (body->GetShapeType())
                    {
                        default:
                        { HE_ENGINE_ASSERT(false, "Unsupported body type"); }

                        case PhysicsBodyShape::Box:
                        {
                            auto extent = body->GetBoxExtent();
                            entry["collisionComponent"]["extent"] = nlohmann::json::array({ extent.x, extent.y, extent.z });
                        } break;

                        case PhysicsBodyShape::Sphere:
                        {
                            entry["collisionComponent"]["radius"] = body->GetSphereRadius();
                        } break;
                            
                        case PhysicsBodyShape::Capsule:
                        {
                            entry["collisionComponent"]["radius"] = body->GetCapsuleRadius();
                            entry["collisionComponent"]["height"] = body->GetCapsuleHeight();
                        } break;
                    }
                }

                field[index++] = entry;
            });
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

        std::ofstream file(path.Data());
        file << j;
    }
}
