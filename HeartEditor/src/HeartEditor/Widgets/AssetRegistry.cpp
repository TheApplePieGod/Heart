#include "hepch.h"
#include "AssetRegistry.h"

#include "Heart/Container/HString.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    void AssetRegistry::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 2.f));
        if (ImGui::BeginTable("RegistryTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("UUID", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Is Resource", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            auto& registry = Heart::AssetManager::GetUUIDRegistry();
            for (auto& pair : registry)
            {
                ImGui::TableNextRow();

                Heart::HString uuid = std::to_string(pair.first);
                Heart::HString id1 = "##" + uuid;
                Heart::HString id2 = id1 + "p";

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(id1.DataUTF8(), (char*)uuid.DataUTF8(), uuid.GetCountUTF8(), ImGuiInputTextFlags_ReadOnly);

                ImGui::TableNextColumn();
                switch (pair.second.Type)
                {
                    default:
                    { ImGui::Text("None"); } break;
                    case Heart::Asset::Type::Texture:
                    { ImGui::Text("Texture"); } break;
                    case Heart::Asset::Type::Shader:
                    { ImGui::Text("Shader"); } break;
                    case Heart::Asset::Type::Mesh:
                    { ImGui::Text("Mesh"); } break;
                    case Heart::Asset::Type::Material:
                    { ImGui::Text("Material"); } break;
                    case Heart::Asset::Type::Scene:
                    { ImGui::Text("Scene"); } break;
                }

                ImGui::TableNextColumn();
                ImGui::Text(pair.second.IsResource ? "true" : "false");

                ImGui::TableNextColumn();
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(id2.DataUTF8(), (char*)pair.second.Path.c_str(), pair.second.Path.size(), ImGuiInputTextFlags_ReadOnly);
                ImGui::EndDisabled();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}