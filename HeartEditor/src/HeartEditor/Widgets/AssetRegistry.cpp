#include "hepch.h"
#include "AssetRegistry.h"

#include "Heart/Container/HString8.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/ImGuiUtils.h"

namespace HeartEditor
{
namespace Widgets
{
    void AssetRegistry::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 2.f));
        if (ImGui::BeginTable("RegistryTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextColumn();
            ImGui::Text("UUID");
            DrawTextFilter(m_UUIDFilter, "##idfilter");

            ImGui::TableNextColumn();
            ImGui::Text("Type");
            DrawAssetTypeFilter();

            ImGui::TableNextColumn();
            ImGui::Text("Is Resource");
            DrawIsResourceFilter();

            ImGui::TableNextColumn();
            ImGui::Text("Path");
            DrawTextFilter(m_PathFilter, "##pathfilter");

            auto& registry = Heart::AssetManager::GetUUIDRegistry();
            for (auto& pair : registry)
            {
                Heart::HString8 uuid = std::to_string(pair.first);

                if (!PassAssetTypeFilter((u32)pair.second.Type) ||
                    !PassIsResourceFilter(pair.second.IsResource) ||
                    !m_UUIDFilter.PassFilter(uuid.Data()) ||
                    !m_PathFilter.PassFilter(pair.second.Path.Data())
                )
                    continue;

                ImGui::TableNextRow();

                Heart::HString8 id1 = "##" + uuid;
                Heart::HString8 id2 = id1 + "p";

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(id1.Data(), (char*)uuid.Data(), uuid.GetCount(), ImGuiInputTextFlags_ReadOnly);

                ImGui::TableNextColumn();
                ImGui::Text(HE_ENUM_TO_STRING(Heart::Asset, pair.second.Type));

                ImGui::TableNextColumn();
                ImGui::Text(pair.second.IsResource ? "true" : "false");

                ImGui::TableNextColumn();
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(id2.Data(), (char*)pair.second.Path.Data(), pair.second.Path.GetCount(), ImGuiInputTextFlags_ReadOnly);
                ImGui::EndDisabled();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void AssetRegistry::DrawFilterPopup(const char* popupName, bool focusOnOpen, std::function<void()>&& drawCallback, std::function<void()>&& clearCallback)
    {
        ImGui::SameLine();

        Heart::HString8 b1id = Heart::HStringView8("V##v") + popupName;
        Heart::HString8 b2id = Heart::HStringView8("X##x") + popupName;

        bool popupOpened = ImGui::Button(b1id.Data());
        if (popupOpened)
            ImGui::OpenPopup(popupName);

        ImGui::SameLine(0.f, 3.f);
        if (ImGui::Button(b2id.Data()))
            clearCallback();

        if (ImGui::BeginPopup(popupName, ImGuiWindowFlags_HorizontalScrollbar))
        {
            drawCallback();
            if (focusOnOpen && popupOpened)
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::EndPopup();
        }
    }

    void AssetRegistry::DrawTextFilter(ImGuiTextFilter& filter, const char* popupName)
    {
        DrawFilterPopup(
            popupName,
            true,
            [&filter] ()
            {
                if (filter.Draw())
                    ImGui::SetKeyboardFocusHere(-1);
            },
            [&filter] ()
            {
                filter.Clear();
            }
        );
    }

    void AssetRegistry::DrawAssetTypeFilter()
    {
        auto& filter = m_AssetTypeFilter;
        DrawFilterPopup(
            "##typefilter",
            false,
            [&filter] ()
            {
                u32 index = 0;
                for (auto str : Heart::Asset::TypeStrings)
                {
                    if (ImGui::Selectable(str, filter == index))
                        filter = index;
                    index++;
                }
            },
            [&filter] ()
            {
                filter = 0;
            }
        );
    }

    void AssetRegistry::DrawIsResourceFilter()
    {
        auto& filter = m_IsResourceFilter;
        DrawFilterPopup(
            "##resfilter",
            false,
            [&filter] ()
            {
                if (ImGui::Selectable("N/A", filter == 0))
                    filter = 0;
                if (ImGui::Selectable("true", filter == 2))
                    filter = 2;
                if (ImGui::Selectable("false", filter == 1))
                    filter = 1;
            },
            [&filter] ()
            {
                filter = 0;
            }
        );
    }

    bool AssetRegistry::PassAssetTypeFilter(u32 type)
    {
        return m_AssetTypeFilter == 0 ||
               m_AssetTypeFilter == type;
    }

    bool AssetRegistry::PassIsResourceFilter(bool isResource)
    {
        return m_IsResourceFilter == 0 ||
               (bool)(m_IsResourceFilter - 1) == isResource;
    }
}
}