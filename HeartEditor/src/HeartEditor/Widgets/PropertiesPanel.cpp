#include "htpch.h"
#include "PropertiesPanel.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    PropertiesPanel::PropertiesPanel()
    {

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

    void PropertiesPanel::OnImGuiRender(Heart::Entity selectedEntity)
    {
        HE_PROFILE_FUNCTION();

        if (selectedEntity.IsValid())
        {
            if (selectedEntity.HasComponent<Heart::NameComponent>())
            {
                auto& nameComponent = selectedEntity.GetComponent<Heart::NameComponent>();

                char buffer[128];
                memset(buffer, 0, sizeof(buffer));
                std::strncpy(buffer, nameComponent.Name.c_str(), sizeof(buffer));
                if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
                {
                    ImGui::SetKeyboardFocusHere(-1);
                    nameComponent.Name = std::string(buffer);
                }

                ImGui::SameLine();
                
                if (ImGui::Button("Add Component"))
                    ImGui::OpenPopup("AddComponent");
                
                if (ImGui::BeginPopup("AddComponent"))
                {
                    if (ImGui::MenuItem("Mesh Component"))
                    {
                        selectedEntity.AddComponent<Heart::MeshComponent>();
                    }

                    ImGui::EndPopup();
                }
            }

            if (selectedEntity.HasComponent<Heart::TransformComponent>())
            {
                bool headerOpen = ImGui::CollapsingHeader("Transform");
                RenderComponentPopup<Heart::MeshComponent>("TransformPopup", selectedEntity, false);
                if (headerOpen)
                {
                    auto& transformComponent = selectedEntity.GetComponent<Heart::TransformComponent>();

                    ImGui::Indent();
                    RenderXYZSlider("Translation  ", &transformComponent.Translation.x, &transformComponent.Translation.y, &transformComponent.Translation.z, -999999.f, 999999.f, 0.1f);
                    RenderXYZSlider("Rotation     ", &transformComponent.Rotation.x, &transformComponent.Rotation.y, &transformComponent.Rotation.z, 0.f, 360.f, 1.f);
                    RenderXYZSlider("Scale        ", &transformComponent.Scale.x, &transformComponent.Scale.y, &transformComponent.Scale.z, 0.f, 999999.f, 0.1f);
                    ImGui::Unindent();

                    selectedEntity.GetScene()->CacheEntityTransform(selectedEntity);
                }
                
            }
            if (selectedEntity.HasComponent<Heart::MeshComponent>())
            {
                bool headerOpen = ImGui::CollapsingHeader("Mesh");
                RenderComponentPopup<Heart::MeshComponent>("MeshPopup", selectedEntity);
                if (headerOpen)
                {
                    auto& transformComponent = selectedEntity.GetComponent<Heart::MeshComponent>();

                    ImGui::Indent();
                    ImGui::Text("Placeholder");
                    ImGui::Unindent();
                }
                
            }
        }
    }
}
}