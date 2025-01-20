#include "hepch.h"
#include "AssetPicker.h"

#include "HeartEditor/Editor.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace HeartEditor
{
    void AssetPicker::OnImGuiRender(
        Heart::UUID selectedAsset,
        Heart::Asset::Type filterType,
        Heart::HStringView8 selectText,
        std::function<void()>&& contextMenuCallback,
        std::function<void(Heart::UUID)>&& selectCallback
    ) {
        HE_PROFILE_FUNCTION();
        
        const auto& UUIDRegistry = Heart::AssetManager::GetUUIDRegistry();

        m_ProcessingIds.Clear();
        for (auto& pair : UUIDRegistry)
        {
            if (pair.second.Type != filterType || !m_PathFilter.PassFilter(pair.second.Path.Data()))
                continue;

            Heart::Asset* asset = Heart::AssetManager::RetrieveAsset(pair.first);
            if (!asset) continue; // Should never happen

            if (!m_NameFilter.PassFilter(asset->GetFilename().Data()))
                continue;

            m_ProcessingIds.Add(pair.first);
        }

        bool valid = selectedAsset && UUIDRegistry.find(selectedAsset) != UUIDRegistry.end();

        m_Picker.OnImGuiRender(
            valid ? UUIDRegistry.at(selectedAsset).Path.Data() : selectText.Data(),
            "Select Asset",
            m_ProcessingIds.Count(), 4,
            [this]()
            {
                ImGui::Text("Preview");

                ImGui::TableNextColumn();
                ImGui::Text("Name");
                Heart::ImGuiUtils::DrawTextFilter(m_NameFilter, "##namefilter");

                ImGui::TableNextColumn();
                ImGui::Text("Type");
                /*
                Heart::ImGuiUtils::DrawStringDropdownFilter(
                    Heart::Asset::TypeStrings,
                    HE_ARRAY_SIZE(Heart::Asset::TypeStrings),
                    m_LevelFilter,
                    "##lvlfilter"
                );
                */

                ImGui::TableNextColumn();
                ImGui::Text("Path");
                Heart::ImGuiUtils::DrawTextFilter(m_PathFilter, "##pathfilter");
            },
            [this](u32 index)
            {
                Heart::UUID id = m_ProcessingIds[index];
                auto& entry = Heart::AssetManager::GetUUIDRegistry().at(id);
                Heart::Asset* asset = Heart::AssetManager::RetrieveAsset(id);

                // TODO: previews

                ImGui::TableNextColumn();
                ImGui::Text("%s", asset->GetFilename().Data());

                ImGui::TableNextColumn();
                ImGui::Text("%s", Heart::Asset::TypeStrings[(u32)asset->GetType()]);

                ImGui::TableNextColumn();
                ImGui::Text("%s", asset->GetPath().Data());
            },
            std::move(contextMenuCallback),
            [this, selectCallback](u32 index)
            {
                Heart::UUID id = m_ProcessingIds[index];
                selectCallback(id);
            }
        );
    }
}
