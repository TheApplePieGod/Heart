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
#include "Heart/Renderer/Renderer.h"
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
        ImGui::Begin(m_Name.c_str(), &m_Open);

        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.IsValid())
        {
            // All entities should have a name component
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

                ImGui::EndPopup();
            }

            RenderTransformComponent();
            RenderMeshComponent();
            RenderLightComponent();
            RenderScriptComponent();
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
                glm::vec3 translation = selectedEntity.GetPosition();
                glm::vec3 rotation = selectedEntity.GetRotation();
                glm::vec3 scale = selectedEntity.GetScale();
                ImGui::Indent();
                RenderXYZSlider("Translation  ", &translation.x, &translation.y, &translation.z, -999999.f, 999999.f, 0.1f);
                RenderXYZSlider("Rotation     ", &rotation.x, &rotation.y, &rotation.z, 0.f, 360.f, 1.f);
                RenderXYZSlider("Scale        ", &scale.x, &scale.y, &scale.z, 0.f, 999999.f, 0.1f);
                ImGui::Unindent();

                selectedEntity.SetTransform(translation, rotation, scale);
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
                            meshComp.Materials.resize(meshAsset->GetMaxMaterials(), 0);
                    }
                );

                // Assign mesh on drop
                Heart::ImGuiUtils::AssetDropTarget(
                    Heart::Asset::Type::Mesh,
                    [&meshComp](const std::string& path)
                    {
                        meshComp.Mesh = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Mesh, path);      
                        auto meshAsset = Heart::AssetManager::RetrieveAsset<Heart::MeshAsset>(meshComp.Mesh);
                        if (meshAsset && meshAsset->IsValid())
                            meshComp.Materials.resize(meshAsset->GetMaxMaterials(), 0);
                    }
                );

                auto meshAsset = Heart::AssetManager::RetrieveAsset<Heart::MeshAsset>(meshComp.Mesh);
                if (meshAsset && meshAsset->IsValid())
                {
                    // Resize the materials to match the max of the mesh
                    // This will add more zeros (defaults) if need be but will not replace old overridden materials
                    if (meshComp.Materials.size() == 0)
                        meshComp.Materials.resize(meshAsset->GetMaxMaterials(), 0);

                    // Selection for each material
                    ImGui::Dummy({ 0.f, 5.f });
                    ImGui::Text("Materials");
                    ImGui::Separator();
                    u32 index = 0;
                    std::string baseName = "Material ";
                    for (auto& materialId : meshComp.Materials)
                    {
                        std::string entryName = baseName + std::to_string(index);

                        // Material picker
                        ImGui::Text(entryName.c_str(), index);
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

                                    std::string path = Heart::FilesystemUtils::SaveAsDialog(Heart::AssetManager::GetAssetsDirectory(), "Export Material", "Material", "hemat");
                                    if (!path.empty())
                                        Heart::MaterialAsset::SerializeMaterial(path, *exportingMaterial);
                                }
                            },
                            [&materialId](Heart::UUID selected) { materialId = selected; }
                        );

                        // Assign material on drop
                        Heart::ImGuiUtils::AssetDropTarget(
                            Heart::Asset::Type::Material,
                            [&materialId](const std::string& path) { materialId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Material, path); }
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
                Heart::HVector<Heart::HString> classes(classDict.size(), false);
                for (auto& pair : classDict)
                    classes.Add(pair.first);

                ImGui::Indent();

                // Show possible assemblies
                ImGui::Text("Class:");
                ImGui::SameLine();
                Heart::ImGuiUtils::StringPicker(
                    classes,
                    scriptComp.Instance.GetScriptClass(),
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
                    [&scriptComp, &classes](u32 index)
                    {
                        scriptComp.Instance = Heart::ScriptInstance(classes[index]);
                        scriptComp.Instance.Instantiate();
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

    void PropertiesPanel::RenderXYZSlider(const std::string& name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step)
    {
        f32 width = ImGui::GetContentRegionAvail().x;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 0.5f));
        if (ImGui::BeginTable(name.c_str(), 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();

            ImU32 xColor = ImGui::GetColorU32(ImVec4(1.f, 0.0f, 0.0f, 1.f));
            ImU32 yColor = ImGui::GetColorU32(ImVec4(0.f, 1.0f, 0.0f, 1.f));
            ImU32 zColor = ImGui::GetColorU32(ImVec4(0.f, 0.0f, 1.0f, 1.f));
            ImU32 textColor = ImGui::GetColorU32(ImVec4(0.f, 0.0f, 0.0f, 1.f));

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, xColor);
            ImGui::Text(" X ");
            
            ImGui::TableSetColumnIndex(2);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            ImGui::DragFloat("##x", x, step, min, max, "%.2f");

            ImGui::TableSetColumnIndex(3);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, yColor);
            ImGui::Text(" Y ");
            
            ImGui::TableSetColumnIndex(4);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            ImGui::DragFloat("##y", y, step, min, max, "%.2f");

            ImGui::TableSetColumnIndex(5);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, zColor);
            ImGui::Text(" Z ");
            
            ImGui::TableSetColumnIndex(6);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            ImGui::DragFloat("##z", z, step, min, max, "%.2f");

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    void PropertiesPanel::RenderScriptField(const Heart::HString& fieldName, Heart::ScriptComponent& scriptComp)
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
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate);
            } break;
            case Heart::Variant::Type::Int:
            {
                int intermediate = value.Int();
                if (ImGui::DragInt(widgetId.DataUTF8(), &intermediate, 1, -1000, 1000))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate);
            } break;
            case Heart::Variant::Type::Float:
            {
                float intermediate = (float)value.Float();
                if (ImGui::DragFloat(widgetId.DataUTF8(), &intermediate, 0.1f, -1000.0f, 1000.f, "%.2f"))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate);
            } break;
            case Heart::Variant::Type::String:
            {
                Heart::HString intermediate = value.String().ToUTF8();
                if (Heart::ImGuiUtils::InputText(widgetId.DataUTF8(), intermediate))
                    scriptComp.Instance.SetFieldValue(fieldName, intermediate.ToUTF16());
            } break;
        }
    }
}
}