#include "hepch.h"
#include "LogList.h"

#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Util/ImGuiUtils.h"

namespace HeartEditor
{
namespace Widgets
{
    void LogList::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 4.f));
        if (ImGui::BeginTable("RegistryTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextColumn();
            ImGui::Text("Timestamp");
            Heart::ImGuiUtils::DrawTextFilter(m_TimestampFilter, "##tsfilter");

            ImGui::TableNextColumn();
            ImGui::Text("Level");
            Heart::ImGuiUtils::DrawStringDropdownFilter(
                Heart::LogListEntry::TypeStrings,
                HE_ARRAY_SIZE(Heart::LogListEntry::TypeStrings),
                m_LevelFilter,
                "##lvlfilter"
            );

            ImGui::TableNextColumn();
            ImGui::Text("Source");
            Heart::ImGuiUtils::DrawTextFilter(m_SourceFilter, "##srcfilter");

            float messageContentSize = 0.f;
            ImGui::TableNextColumn();
            ImGui::Text("Message");
            messageContentSize = ImGui::GetContentRegionMax().x;
            Heart::ImGuiUtils::DrawTextFilter(m_MessageFilter, "##msgfilter");

            Heart::Logger::LockLogList();
            auto& logList = Heart::Logger::GetLogList();
            Heart::HVector<ListEntry> filteredEntries;
            for (int i = logList.size() - 1; i >= 0; i--)
            {
                auto& entry = logList[i];
                if (PassLevelFilter((u32)entry.Level) &&
                    m_TimestampFilter.PassFilter(entry.Timestamp.c_str()) &&
                    m_SourceFilter.PassFilter(entry.Source.c_str()) &&
                    m_MessageFilter.PassFilter(entry.Message.c_str())
                )
                {
                    u32 lineIndex = 0;
                    auto lines = Heart::HStringView8(entry.Message.data(), entry.Message.size()).Split("\n");
                    for (auto line : lines)
                    {
                        float textWidth = ImGui::CalcTextSize(line.Data(), line.Data() + line.Count()).x;
                        if (textWidth == 0.f) continue;
                        int lineCount = (int)pow(2.f, ceil(textWidth / messageContentSize)) - 1;
                        int charCount = (int)line.Count() / lineCount;
                        
                        for (int i = 0; i < lineCount; i++)
                        {
                            auto substr = line.Substr(charCount * i, charCount);
                            Heart::LogListEntry partialEntry(
                                entry.Level,
                                lineIndex == 0 ? entry.Timestamp : "",
                                lineIndex == 0 ? entry.Source : "",
                                std::string_view(substr.Data(), substr.Count())
                            );
                            filteredEntries.Add({ partialEntry, lineIndex != 0 });
                        
                            lineIndex++;
                        }
                    }
                }
            }
            Heart::Logger::UnlockLogList();

            if (filteredEntries.Count() > 0)
            {
                ImGuiListClipper clipper; // For virtualizing the list
                clipper.Begin(filteredEntries.Count());
                while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    {
                        if (i >= filteredEntries.Count()) break;
                        auto& entry = filteredEntries[i];

                        ImVec4 rowColor = { 1.f, 1.f, 1.f, 0.5f };
                        switch (entry.Entry.Level)
                        {
                            default: break;
                            case Heart::LogLevel::Debug: { rowColor = { 0.5f, 1.0f, 1.0f, 1.f }; } break;
                            case Heart::LogLevel::Info: { rowColor = { 0.15f, 1.0f, 0.3f, 1.f }; } break;
                            case Heart::LogLevel::Warn: { rowColor = { 1.0f, 1.0f, 0.3f, 1.f }; } break;
                            case Heart::LogLevel::Error: { rowColor = { 1.0f, 0.3f, 0.2f, 1.f }; } break;
                            case Heart::LogLevel::Critical: { rowColor = { 1.0f, 0.0f, 0.0f, 1.f }; } break;
                        }
                        
                        ImGui::TableNextRow();
                        
                        if (entry.Partial)
                            ImGui::TableSetColumnIndex(3);
                        else
                        {
                            ImGui::TableNextColumn();
                            ImGui::TextColored(rowColor, entry.Entry.Timestamp.c_str());
                            
                            ImGui::TableNextColumn();
                            ImGui::TextColored(rowColor, HE_ENUM_TO_STRING(Heart::LogListEntry, entry.Entry.Level));
                            
                            ImGui::TableNextColumn();
                            ImGui::TextColored(rowColor, entry.Entry.Source.c_str());
                            
                            ImGui::TableNextColumn();
                        }
                        
                        ImGui::TextColored(rowColor, entry.Entry.Message.c_str());
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool LogList::PassLevelFilter(u32 level)
    {
        return level >= m_LevelFilter;
    }
}
}
