#include "hepch.h"
#include "Picker.h"

#include "HeartEditor/Editor.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace HeartEditor
{
    void Picker::OnImGuiRender(
        Heart::HStringView8 buttonText,
        Heart::HStringView8 popupName,
        u32 rowCount,
        u32 colCount,
        std::function<void()>&& renderHeader,
        std::function<void(u32)>&& renderRow,
        std::function<void()>&& contextMenuCallback,
        std::function<void(u32)>&& selectCallback
    ) {
        HE_PROFILE_FUNCTION();

        bool popupOpened = ImGui::Button(buttonText.Data());
        if (popupOpened)
            ImGui::OpenPopup(popupName.Data());
        
        // right click menu
        if (contextMenuCallback && ImGui::BeginPopupContextItem("ctx"))
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }
        
        bool open = false;
        ImVec2 windowSize = ImGui::GetMainViewport()->Size;
        ImVec2 popupSize = { windowSize.x / 2, windowSize.y / 2 };
        ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
        ImGui::SetNextWindowPos({ windowPos.x + windowSize.x / 4, windowPos.y + windowSize.y / 4 });
        ImGui::SetNextWindowSize(popupSize);
        ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginPopupModal(popupName.Data(), 0, popupFlags))
        {
            open = true;

            ImVec2 mousePos = ImGui::GetMousePos();

            ImVec2 padding = { 4.f, 4.f };
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, padding);
            ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
            ImGuiID tableId = ImGui::GetID("PickerTable");
            if (ImGui::BeginTable("PickerTable", colCount, tableFlags))
            {
                ImGui::TableNextColumn();
                renderHeader();

                ImGuiListClipper clipper;
                clipper.Begin(rowCount, m_ItemsHeight);
                while (clipper.Step())
                {
                    for (u32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    {
                        ImGui::PushID(i);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImVec2 rowMin = ImGui::GetCursorScreenPos();
                        rowMin.y -= padding.y;

                        if (m_ItemsHeight != -1.f)
                        {
                            ImVec2 rowMax = { rowMin.x + popupSize.x, rowMin.y + m_ItemsHeight };
                            bool hovered = mousePos.x >= rowMin.x && mousePos.x <= rowMax.x &&
                                mousePos.y >= rowMin.y && mousePos.y <= rowMax.y;

                            if (hovered)
                                ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, IM_COL32(128, 128, 128, 60));

                            ImGuiTable* table = ImGui::GetCurrentContext()->Tables.GetByKey(tableId);
                            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && table->ResizedColumn == -1)
                            {
                                selectCallback(i);
                                ImGui::CloseCurrentPopup();
                            }
                        }

                        renderRow(i);

                        ImGui::PopID();
                    }
                }

                m_ItemsHeight = clipper.ItemsHeight;

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            if (ImGui::IsKeyReleased(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
