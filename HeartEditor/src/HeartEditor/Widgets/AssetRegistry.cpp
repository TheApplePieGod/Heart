#include "hepch.h"
#include "AssetRegistry.h"

#include "Heart/Container/HString8.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/ImGuiUtils.h"

namespace HeartEditor
{
namespace Widgets
{
    struct FilteredElement
    {
        Heart::HString8 UUIDStr;
        Heart::AssetManager::UUIDEntry Entry;
        bool Loaded;
    };

    void AssetRegistry::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        const int columnCount = 5;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 2.f));
        if (ImGui::BeginTable("RegistryTable", columnCount, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextColumn();
            ImGui::Text("UUID");
            Heart::ImGuiUtils::DrawTextFilter(m_UUIDFilter, "##idfilter");

            ImGui::TableNextColumn();
            ImGui::Text("Type");
            Heart::ImGuiUtils::DrawStringDropdownFilter(
                Heart::Asset::TypeStrings,
                HE_ARRAY_SIZE(Heart::Asset::TypeStrings),
                m_AssetTypeFilter,
                "##typefilter"
            );

            ImGui::TableNextColumn();
            ImGui::Text("Is Resource");
            DrawIsResourceFilter();

            ImGui::TableNextColumn();
            ImGui::Text("Is Loaded");
            DrawIsLoadedFilter();

            ImGui::TableNextColumn();
            ImGui::Text("Path");
            Heart::ImGuiUtils::DrawTextFilter(m_PathFilter, "##pathfilter");

            auto& registry = Heart::AssetManager::GetUUIDRegistry();
            Heart::HVector<FilteredElement> filteredRegistry;
            for (auto& pair : registry)
            {
                bool loaded = Heart::AssetManager::RetrieveAsset(pair.first)->IsLoaded();
                Heart::HString8 uuid = std::to_string(pair.first);
                if (PassAssetTypeFilter((u32)pair.second.Type) &&
                    PassIsResourceFilter(pair.second.IsResource) &&
                    PassIsLoadedFilter(loaded) &&
                    m_UUIDFilter.PassFilter(uuid.Data()) &&
                    m_PathFilter.PassFilter(pair.second.Path.Data())
                )
                    filteredRegistry.Add({ uuid, pair.second, loaded });
            }

            ImGuiListClipper clipper; // For virtualizing the list
            clipper.Begin(filteredRegistry.Count());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto& elem = filteredRegistry[i];
                    Heart::HStringView8 uuid = elem.UUIDStr;

                    ImGui::TableNextRow();

                    Heart::HString8 id1 = Heart::HStringView8("##") + uuid;
                    Heart::HString8 id2 = id1 + "p";

                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputText(id1.Data(), (char*)uuid.Data(), uuid.Count(), ImGuiInputTextFlags_ReadOnly);

                    ImGui::TableNextColumn();
                    ImGui::Text(HE_ENUM_TO_STRING(Heart::Asset, elem.Entry.Type));

                    ImGui::TableNextColumn();
                    ImGui::Text(elem.Entry.IsResource ? "true" : "false");

                    ImGui::TableNextColumn();
                    ImGui::Text(elem.Loaded ? "true" : "false");

                    ImGui::TableNextColumn();
                    ImGui::BeginDisabled();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputText(id2.Data(), (char*)elem.Entry.Path.Data(), elem.Entry.Path.Count(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::EndDisabled();
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void AssetRegistry::DrawIsResourceFilter()
    {
        auto& filter = m_IsResourceFilter;
        Heart::ImGuiUtils::DrawFilterPopup(
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

    void AssetRegistry::DrawIsLoadedFilter()
    {
        auto& filter = m_IsLoadedFilter;
        Heart::ImGuiUtils::DrawFilterPopup(
            "##loadfilter",
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

    bool AssetRegistry::PassIsLoadedFilter(bool isLoaded)
    {
        return m_IsLoadedFilter == 0 ||
               (bool)(m_IsLoadedFilter - 1) == isLoaded;
    }
}
}
