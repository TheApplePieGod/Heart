#include "hepch.h"
#include "LogList.h"

#include "Heart/Container/HString8.h"
#include "Heart/Container/HVector.hpp"
#include "Heart/Util/ImGuiUtils.h"

namespace HeartEditor
{
namespace Widgets
{
    void LogList::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 4.f));
        if (ImGui::BeginTable("RegistryTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            bool shouldRebuild = false;

            ImGui::TableNextColumn();
            ImGui::Text("Timestamp");
            shouldRebuild |= Heart::ImGuiUtils::DrawTextFilter(m_TimestampFilter, "##tsfilter");

            ImGui::TableNextColumn();
            ImGui::Text("Level");
            shouldRebuild |= Heart::ImGuiUtils::DrawStringDropdownFilter(
                Heart::LogListEntry::TypeStrings,
                HE_ARRAY_SIZE(Heart::LogListEntry::TypeStrings),
                m_LevelFilter,
                "##lvlfilter"
            );

            ImGui::TableNextColumn();
            ImGui::Text("Source");
            shouldRebuild |= Heart::ImGuiUtils::DrawTextFilter(m_SourceFilter, "##srcfilter");

            const float messageContentPadding = 50.f;
            ImGui::TableNextColumn();
            ImGui::Text("Message");
            float messageContentSize = std::max(ImGui::GetContentRegionAvail().x, 1.f);
            shouldRebuild |= Heart::ImGuiUtils::DrawTextFilter(m_MessageFilter, "##msgfilter");

            // Also rebuild if we resize the widget.
            // TODO: this is scuffed, and we really should just figure out how to properly
            // support multiline logs 
            shouldRebuild |= fabsf(messageContentSize - m_LastWidth) > 10.f;

            if (shouldRebuild)
            {
                m_FilteredEntries.Clear();
                m_LastWidth = messageContentSize;
            }

            Heart::Logger::LockLogList();
            auto& logList = Heart::Logger::GetLogList();

            m_ProcessingEntries.Clear();
            for (int i = logList.size() - 1; i >= 0; i--)
            {
                auto& entry = logList[i];

                // Sorted by newest first first
                if (!m_FilteredEntries.IsEmpty() && entry.Id <= m_FilteredEntries.Front().Entry.Id)
                    break;

                if (m_ProcessingEntries.Count() >= m_MaxEntries)
                    break;

                if (!PassLevelFilter((u32)entry.Level) ||
                    !m_TimestampFilter.PassFilter(entry.Timestamp.c_str()) ||
                    !m_SourceFilter.PassFilter(entry.Source.c_str()) ||
                    !m_MessageFilter.PassFilter(entry.Message.c_str())
                )
                    continue;

                u32 lineIndex = 0;
                auto lines = Heart::HStringView8(entry.Message.data(), entry.Message.size()).Split("\n");
                for (auto line : lines)
                {
                    float textWidth = ImGui::CalcTextSize(line.Data(), line.Data() + line.Count()).x + messageContentPadding;
                    if (textWidth == 0.f) continue;
                    int lineCount = (textWidth / messageContentSize) + 1;
                    int charCount = (int)line.Count() / lineCount;
                    
                    for (int i = 0; i < lineCount; i++)
                    {
                        auto substr = line.Substr(charCount * i, charCount);
                        Heart::LogListEntry partialEntry(
                            entry.Id,
                            entry.Level,
                            lineIndex == 0 ? entry.Timestamp : "",
                            lineIndex == 0 ? entry.Source : "",
                            std::string_view(substr.Data(), substr.Count())
                        );
                        m_ProcessingEntries.Add(
                            { std::move(partialEntry), lineIndex != 0 }
                        );
                    
                        lineIndex++;
                    }
                }
            }

            Heart::Logger::UnlockLogList();

            m_FilteredEntries.Insert(m_ProcessingEntries, 0);
            if (m_FilteredEntries.Count() > m_MaxEntries)
                m_FilteredEntries.Resize(m_MaxEntries);

            if (m_FilteredEntries.Count() > 0)
            {
                ImGuiListClipper clipper; // For virtualizing the list
                clipper.Begin(m_FilteredEntries.Count());
                while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    {
                        if (i >= m_FilteredEntries.Count()) break;
                        auto& entry = m_FilteredEntries[i];

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
