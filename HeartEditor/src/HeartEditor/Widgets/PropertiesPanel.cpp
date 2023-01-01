#include "hepch.h"
#include "PropertiesPanel.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Scripting/ScriptingEngine.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Container/Variant.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "Heart/Util/FilesystemUtils.h"
#include "imgui/imgui_internal.h"
#include "glm/gtc/type_ptr.hpp"

#include "HeartEditor/Widgets/MaterialEditor.h"

namespace HeartEditor
{
namespace Widgets
{
    void PropertiesPanel::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.IsValid())
        {
            // All entities should have a name & id component
            auto& nameComponent = selectedEntity.GetComponent<Heart::NameComponent>();
            Heart::ImGuiUtils::InputText("##Name", nameComponent.Name);

            ImGui::SameLine();
            
            if (ImGui::Button("Add Component"))
                ImGui::OpenPopup("AddComponent");
            
            if (ImGui::BeginPopup("AddComponent"))
            {
                if (ImGui::MenuItem("Mesh Component"))
                    selectedEntity.AddComponent<Heart::MeshComponent>();
                if (ImGui::MenuItem("Light Component"))
                    selectedEntity.AddComponent<Heart::LightComponent>();
                if (ImGui::MenuItem("Script Component"))
                    selectedEntity.AddComponent<Heart::ScriptComponent>();
                if (ImGui::MenuItem("Camera Component"))
                    selectedEntity.AddComponent<Heart::CameraComponent>();
                if (ImGui::MenuItem("Rigid Body Component"))
                {
                    auto body = Heart::PhysicsBody::CreateDefaultBody((void*)(intptr_t)selectedEntity.GetUUID());
                    selectedEntity.AddComponent<Heart::RigidBodyComponent>(body);
                }
                    
                ImGui::EndPopup();
            }
            
            auto id = selectedEntity.GetUUID();
            ImGui::Text("Id: %llu", (u64)id);
            
            RenderTransformComponent();
            RenderMeshComponent();
            RenderLightComponent();
            RenderScriptComponent();
            RenderCameraComponent();
            RenderRigidBodyComponent();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void PropertiesPanel::RenderTransformComponent()
    {
        // All entities should have a transform component but double check just in case
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::TransformComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Transform");
            if (!RenderComponentPopup<Heart::TransformComponent>("TransformPopup", false) && headerOpen)
            {
                bool modified = false;
                glm::vec3 translation = selectedEntity.GetPosition();
                glm::vec3 rotation = selectedEntity.GetRotation();
                glm::vec3 scale = selectedEntity.GetScale();
                ImGui::Indent();
                modified |= Heart::ImGuiUtils::XYZSlider("Translation  ", &translation.x, &translation.y, &translation.z, -999999.f, 999999.f, 0.1f);
                modified |= Heart::ImGuiUtils::XYZSlider("Rotation     ", &rotation.x, &rotation.y, &rotation.z, -360.f, 360.f, 1.f);
                modified |= Heart::ImGuiUtils::XYZSlider("Scale        ", &scale.x, &scale.y, &scale.z, 0.f, 999999.f, 0.1f);
                ImGui::Unindent();
                
                if (modified)
                {
                    // Apply a rot delta rather than setting when we are dealing with physics entities
                    // because of quat weirdness
                    if (selectedEntity.HasComponent<Heart::RigidBodyComponent>())
                    {
                        selectedEntity.ApplyRotation(rotation - selectedEntity.GetRotation());
                        selectedEntity.SetTransform(translation, selectedEntity.GetRotation(), scale);
                    }
                    else
                        selectedEntity.SetTransform(translation, rotation, scale);
                }
            }
        }
    }

    void PropertiesPanel::RenderMeshComponent()
    {
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::MeshComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Mesh"); 
            if (!RenderComponentPopup<Heart::MeshComponent>("MeshPopup") && headerOpen)
            {
                auto& meshComp = selectedEntity.GetComponent<Heart::MeshComponent>();
                const auto& UUIDRegistry = Heart::AssetManager::GetUUIDRegistry();

                ImGui::Indent();

                // Mesh picker
                ImGui::Text("Mesh Asset:");
                ImGui::SameLine();
                Heart::ImGuiUtils::AssetPicker(
                    Heart::Asset::Type::Mesh,
                    meshComp.Mesh,
                    "NULL",
                    "MeshSelect",
                    m_MeshTextFilter,
                    [&meshComp]()
                    {
                        if (!meshComp.Mesh)
                            return;

                        if (ImGui::MenuItem("Clear"))
                            meshComp.Mesh = 0;
                    },
                    [&meshComp](Heart::UUID selected)
                    {
                        meshComp.Mesh = selected;

                        auto meshAsset = Heart::AssetManager::RetrieveAsset<Heart::MeshAsset>(meshComp.Mesh);
                        if (meshAsset && meshAsset->IsValid())
                            meshComp.Materials.Resize(meshAsset->GetMaxMaterials(), false);
                    }
                );

                // Assign mesh on drop
                Heart::ImGuiUtils::AssetDropTarget(
                    Heart::Asset::Type::Mesh,
                    [&meshComp](const Heart::HStringView8& path)
                    {
                        meshComp.Mesh = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, path);      
                        auto meshAsset = Heart::AssetManager::RetrieveAsset<Heart::MeshAsset>(meshComp.Mesh);
                        if (meshAsset && meshAsset->IsValid())
                            meshComp.Materials.Resize(meshAsset->GetMaxMaterials(), false);
                    }
                );

                auto meshAsset = Heart::AssetManager::RetrieveAsset<Heart::MeshAsset>(meshComp.Mesh);
                if (meshAsset && meshAsset->IsValid())
                {
                    // Resize the materials to match the max of the mesh
                    // This will add more zeros (defaults) if need be but will not replace old overridden materials
                    if (meshComp.Materials.Count() == 0)
                        meshComp.Materials.Resize(meshAsset->GetMaxMaterials(), false);

                    // Selection for each material
                    ImGui::Dummy({ 0.f, 5.f });
                    ImGui::Text("Materials");
                    ImGui::Separator();
                    u32 index = 0;
                    Heart::HStringView8 baseName = "Material ";
                    for (auto& materialId : meshComp.Materials)
                    {
                        Heart::HString8 entryName = baseName + Heart::HStringView8(std::to_string(index));

                        // Material picker
                        ImGui::Text(entryName.Data(), index);
                        ImGui::SameLine();
                        Heart::ImGuiUtils::AssetPicker(
                            Heart::Asset::Type::Material,
                            materialId,
                            "DEFAULT",
                            entryName,
                            m_MaterialTextFilter,
                            [&materialId, &meshAsset, index]()
                            {
                                // Context menu per material
                                if (materialId)
                                {
                                    if (ImGui::MenuItem("Clear"))
                                        materialId = 0;
                                    if (ImGui::MenuItem("Open in Editor"))
                                        ((Widgets::MaterialEditor&)Editor::GetWindow("Material Editor")).SetSelectedMaterial(materialId);
                                }
                                if (ImGui::MenuItem("Export to File"))
                                {
                                    Heart::Material* exportingMaterial = &meshAsset->GetDefaultMaterials()[index]; // default material
                                    if (materialId != 0)
                                    {
                                        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(materialId);
                                        if (materialAsset && materialAsset->IsValid())
                                            exportingMaterial = &materialAsset->GetMaterial();
                                    }

                                    Heart::HString8 path = Heart::FilesystemUtils::SaveAsDialog(Heart::AssetManager::GetAssetsDirectory(), "Export Material", "Material", "hemat");
                                    if (!path.IsEmpty())
                                        Heart::MaterialAsset::SerializeMaterial(path, *exportingMaterial);
                                }
                            },
                            [&materialId](Heart::UUID selected) { materialId = selected; }
                        );

                        // Assign material on drop
                        Heart::ImGuiUtils::AssetDropTarget(
                            Heart::Asset::Type::Material,
                            [&materialId](const Heart::HStringView8& path) { materialId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Material, path); }
                        );

                        index++;
                    }
                }
                else
                    ImGui::TextColored({ 0.9f, 0.1f, 0.1f, 1.f }, "Invalid Mesh");
                
                ImGui::Unindent();
            }
        }
    }

    void PropertiesPanel::RenderLightComponent()
    {
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::LightComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Light");
            if (!RenderComponentPopup<Heart::LightComponent>("PointLightPopup", true) && headerOpen)
            {
                auto& lightComp = selectedEntity.GetComponent<Heart::LightComponent>();

                ImGui::Indent();

                ImGui::Text("Light Type:");
                ImGui::SameLine();
                bool popupOpened = ImGui::Button(HE_ENUM_TO_STRING(Heart::LightComponent, lightComp.LightType));
                if (popupOpened)
                    ImGui::OpenPopup("LightTypeSelect");
            
                if (ImGui::BeginPopup("LightTypeSelect"))
                {
                    if (ImGui::MenuItem("Disabled"))
                        lightComp.LightType = Heart::LightComponent::Type::Disabled;
                    if (ImGui::MenuItem("Directional"))
                        lightComp.LightType = Heart::LightComponent::Type::Directional;
                    if (ImGui::MenuItem("Point"))
                        lightComp.LightType = Heart::LightComponent::Type::Point;
                    ImGui::EndPopup();
                }

                ImGui::Text("Color");
                ImGui::SameLine();
                ImGui::ColorEdit3("##Color", (float*)&lightComp.Color);

                ImGui::Text("Intensity");
                ImGui::SameLine();
                ImGui::DragFloat("##Intensity", &lightComp.Color.a, 0.5f, 0.f, 100.f);

                ImGui::Text("Attenuation");
                ImGui::Separator();

                ImGui::Text("Constant");
                ImGui::SameLine();
                ImGui::DragFloat("##ConstantAtten", &lightComp.ConstantAttenuation, 0.005f, 0.f, 2.f);

                ImGui::Text("Linear");
                ImGui::SameLine();
                ImGui::DragFloat("##LinearAtten", &lightComp.LinearAttenuation, 0.005f, 0.f, 2.f);

                ImGui::Text("Quadratic");
                ImGui::SameLine();
                ImGui::DragFloat("##QuadAtten", &lightComp.QuadraticAttenuation, 0.005f, 0.f, 2.f);

                ImGui::Unindent();
            }
        }
    }

    void PropertiesPanel::RenderScriptComponent()
    {
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::ScriptComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Script");
            if (!RenderComponentPopup<Heart::ScriptComponent>("ScriptPopup", true) && headerOpen)
            {
                Heart::UUID uuid = selectedEntity.GetUUID();
                auto& scriptComp = selectedEntity.GetComponent<Heart::ScriptComponent>();

                // Get keys of instantiable classes (names)
                auto& classDict = Heart::ScriptingEngine::GetInstantiableClasses();
                Heart::HVector<Heart::HString8> classes(classDict.size(), false);
                for (auto& pair : classDict)
                    classes.Add(pair.first.ToUTF8());

                ImGui::Indent();

                // Show possible assemblies
                ImGui::Text("Class:");
                ImGui::SameLine();
                Heart::ImGuiUtils::StringPicker(
                    classes,
                    scriptComp.Instance.GetScriptClass().GetViewUTF8(),
                    "None",
                    "Script",
                    m_ScriptTextFilter,
                    [&scriptComp]()
                    {
                        if (!scriptComp.Instance.IsInstantiable())
                            return;

                        if (ImGui::MenuItem("Clear"))
                            scriptComp.Instance.Clear();
                    },
                    [&scriptComp, &classes, selectedEntity](u32 index)
                    {
                        scriptComp.Instance.Destroy();
                        scriptComp.Instance = Heart::ScriptInstance(classes[index].ToHString());
                        scriptComp.Instance.Instantiate(selectedEntity);
                        scriptComp.Instance.OnConstruct();
                    }
                );

                if (scriptComp.Instance.IsAlive())
                {
                    ImGui::Dummy({ 0.f, 5.f });
                    ImGui::Text("Properties");
                    ImGui::Separator();
                    auto& fields = Heart::ScriptingEngine::GetInstantiableClass(scriptComp.Instance.GetScriptClass()).GetSerializableFields();
                    for (auto& val : fields)
                        RenderScriptField(val, scriptComp);
                }

                ImGui::Unindent();
            }
        }
    }

    void PropertiesPanel::RenderCameraComponent()
    {
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::CameraComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Camera");
            if (!RenderComponentPopup<Heart::CameraComponent>("CameraPopup", true) && headerOpen)
            {
                auto& camComp = selectedEntity.GetComponent<Heart::CameraComponent>();

                ImGui::Indent();

                ImGui::Text("Primary:");
                ImGui::SameLine();
                bool primary = selectedEntity.HasComponent<Heart::PrimaryCameraComponent>();
                if (ImGui::Checkbox("##primary", &primary))
                    selectedEntity.SetIsPrimaryCameraEntity(primary);

                ImGui::Text("FOV");
                ImGui::SameLine();
                ImGui::DragFloat("##fov", &camComp.FOV, 0.5f, 0.f, 259.f);

                ImGui::Text("Near Clip");
                ImGui::SameLine();
                ImGui::DragFloat("##nearclip", &camComp.NearClipPlane, 0.005f, 0.f, 1000.f);

                ImGui::Text("Far Clip");
                ImGui::SameLine();
                ImGui::DragFloat("##farclip", &camComp.FarClipPlane, 0.005f, 0.f, 1000.f);

                ImGui::Unindent();
            }
        }
    }

    void PropertiesPanel::RenderRigidBodyComponent()
    {
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.HasComponent<Heart::RigidBodyComponent>())
        {
            bool headerOpen = ImGui::CollapsingHeader("Rigid Body");
            if (!RenderComponentPopup<Heart::RigidBodyComponent>("RBPopup", true) && headerOpen)
            {
                auto& bodyComp = selectedEntity.GetComponent<Heart::RigidBodyComponent>();
                Heart::PhysicsBody* body = selectedEntity.GetPhysicsBody();
                
                ImGui::Indent();

                Heart::PhysicsBodyCreateInfo bodyInfo = body->GetInfo();
                ImGui::Text("Mass:");
                ImGui::SameLine();
                bool recreate = ImGui::DragFloat("##mass", &bodyInfo.Mass, 0.25f, 0.f, 1000.f);

                Heart::PhysicsBody::Type bodyType = body->GetBodyType();
                ImGui::Text("Body Type:");
                ImGui::SameLine();
                bool popupOpened = ImGui::Button(HE_ENUM_TO_STRING(Heart::PhysicsBody, bodyType));
                if (popupOpened)
                    ImGui::OpenPopup("BodyTypeSelect");
            
                if (ImGui::BeginPopup("BodyTypeSelect"))
                {
                    if (ImGui::MenuItem("Box") && bodyType != Heart::PhysicsBody::Type::Box)
                        selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateBoxShape(bodyInfo, { 0.5f, 0.5f, 0.5f }));
                    if (ImGui::MenuItem("Sphere") && bodyType != Heart::PhysicsBody::Type::Sphere)
                        selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateSphereShape(bodyInfo, 0.5f));
                    if (ImGui::MenuItem("Capsule") && bodyType != Heart::PhysicsBody::Type::Capsule)
                        selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateCapsuleShape(bodyInfo, 0.5f, 1.f));
                    ImGui::EndPopup();
                }
                
                if (ImGui::CollapsingHeader("Collision Channels"))
                {
                    ImGui::Indent();
                    recreate |= RenderCollisionChannels("CH", bodyInfo.CollisionChannels);
                    ImGui::Unindent();
                }
                    
                if (ImGui::CollapsingHeader("Collision Mask"))
                {
                    ImGui::Indent();
                    recreate |= RenderCollisionChannels("MSK", bodyInfo.CollisionMask);
                    ImGui::Unindent();
                }
    
                ImGui::Dummy({ 0.f, 5.f });
                ImGui::Text("Shape Properties");
                ImGui::Separator();

                switch (body->GetBodyType())
                {
                    default: break;
                    
                    case Heart::PhysicsBody::Type::Box:
                    {
                        auto extent = body->GetBoxExtent();
                        if (recreate || Heart::ImGuiUtils::XYZSlider(
                            "Half Extent  ",
                            &extent.x,
                            &extent.y,
                            &extent.z,
                            0.f,
                            1000.f,
                            0.25f)
                        )
                            selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateBoxShape(bodyInfo, extent));
                    } break;

                    case Heart::PhysicsBody::Type::Sphere:
                    {
                        float radius = body->GetSphereRadius();
                        ImGui::Text("Radius");
                        ImGui::SameLine();
                        if (recreate || ImGui::DragFloat("##radius", &radius, 0.25f, 0.f, 1000.f))
                            selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateSphereShape(bodyInfo, radius));
                    } break;
                        
                    case Heart::PhysicsBody::Type::Capsule:
                    {
                        float radius = body->GetCapsuleRadius();
                        float height = body->GetCapsuleHeight();
                        ImGui::Text("Radius");
                        ImGui::SameLine();
                        recreate |= ImGui::DragFloat("##radius", &radius, 0.25f, 0.f, 1000.f);
                        
                        ImGui::Text("Height");
                        ImGui::SameLine();
                        recreate |= ImGui::DragFloat("##height", &height, 0.25f, 0.f, 1000.f);
                        
                        if (recreate)
                            selectedEntity.ReplacePhysicsBody(Heart::PhysicsBody::CreateCapsuleShape(bodyInfo, radius, height));
                    } break;
                }
                 
                ImGui::Unindent();
            }
        }
    }

    void PropertiesPanel::RenderScriptField(Heart::HStringView fieldName, Heart::ScriptComponent& scriptComp)
    {
        ImGui::Text(fieldName.DataUTF8());
        ImGui::SameLine();

        Heart::Variant value = scriptComp.Instance.GetFieldValue(fieldName);
        Heart::HString widgetId = "##" + fieldName;
        switch (value.GetType())
        {
            default:
            {
                ImGui::Text("[Edit not implemented]");
            } break;
            case Heart::Variant::Type::Bool:
            {
                bool intermediate = value.Bool();
                if (ImGui::Checkbox(widgetId.DataUTF8(), &intermediate))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate, true);
            } break;
            case Heart::Variant::Type::Int:
            {
                constexpr s64 min = std::numeric_limits<s64>::lowest();
                constexpr s64 max = std::numeric_limits<s64>::max();
                s64 intermediate = value.Int();
                if (ImGui::DragScalar(
                    widgetId.DataUTF8(),
                    ImGuiDataType_S64,
                    &intermediate,
                    1.f, &min, &max
                ))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate, true);
            } break;
            case Heart::Variant::Type::UInt:
            {
                constexpr u64 min = std::numeric_limits<u64>::lowest();
                constexpr u64 max = std::numeric_limits<u64>::max();
                u64 intermediate = value.UInt();
                if (ImGui::DragScalar(
                    widgetId.DataUTF8(),
                    ImGuiDataType_U64,
                    &intermediate,
                    1.f, &min, &max
                ))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate, true);
            } break;
            case Heart::Variant::Type::Float:
            {
                constexpr float min = std::numeric_limits<float>::lowest();
                constexpr float max = std::numeric_limits<float>::max();
                float intermediate = (float)value.Float();
                if (ImGui::DragFloat(
                    widgetId.DataUTF8(),
                    &intermediate,
                    0.1f, min, max,
                    "%.2f"
                ))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate, true);
            } break;
            case Heart::Variant::Type::String:
            {
                Heart::HString intermediate = value.String().Convert(Heart::HString::Encoding::UTF8);
                if (Heart::ImGuiUtils::InputText(widgetId.DataUTF8(), intermediate))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate.Convert(Heart::HString::Encoding::UTF16), true);
            } break;
        }
    }

    bool PropertiesPanel::RenderCollisionChannels(Heart::HStringView8 id, u32& mask)
    {
        constexpr std::array<const char*, 3> names = {
            "Default",
            "Static",
            "Dynamic"
        };
        auto idStart = Heart::HString8("##") + id;
        bool modified = false;
        for (u32 i = 0; i < 3; i++)
        {
            bool set = mask & HE_BIT(i);
            ImGui::Text(names[i]);
            ImGui::SameLine();
            modified |= ImGui::Checkbox((idStart + std::to_string(i)).Data(), &set);
            if (set)
                mask |= HE_BIT(i);
            else
                mask &= ~HE_BIT(i);
        }
        
        for (auto& pair : Heart::PhysicsBody::GetCustomCollisionChannels())
        {
            bool set = mask & pair.second;
            ImGui::Text(pair.first.Data());
            ImGui::SameLine();
            modified |= ImGui::Checkbox((idStart + pair.first).Data(), &set);
            if (set)
                mask |= pair.second;
            else
                mask &= ~pair.second;
        }
        
        return modified;
    }
}
}
