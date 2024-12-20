#include "hepch.h"
#include "SceneHierarchyPanel.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Container/HString.h"
#include "Heart/Scene/Components.h"
#include "Heart/Scene/Entity.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    //typedef std::pair<Heart::HStringView, entt::entity> TreeNodeData;
    void SceneHierarchyPanel::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        // Only top level components
        auto& activeScene = Editor::GetActiveScene();
        auto view = activeScene.GetRegistry().view<Heart::IdComponent, Heart::NameComponent>(entt::exclude<Heart::ParentComponent>);

        // Order by name
        m_RootNodes.Clear();
        m_SortingPairs.Clear();
        for (auto handle : view)
            m_SortingPairs.Add({ view.get<Heart::NameComponent>(handle).Name, handle });
        std::sort(m_SortingPairs.Begin(), m_SortingPairs.End());
        for (auto& pair : m_SortingPairs)
        {
            Heart::Entity entity(&activeScene, pair.second);
            m_RootNodes.Add({ entity.GetUUID(), (void*)pair.second, entity.HasChildren() });
        }

        m_Tree.Rebuild(
            m_RootNodes,
            [](const auto& node)
            {
                Heart::Entity entity(&Editor::GetActiveScene(), (entt::entity)(uintptr_t)node.UserData);
                Heart::HVector<VirtualizedTree::Node> nodes;

                auto& children = entity.GetChildren();
                nodes.Reserve(children.Count());
                for (auto uuid : children)
                {
                    auto child = entity.GetScene()->GetEntityFromUUID(uuid);
                    nodes.Add({ child.GetUUID(), (void*)child.GetHandle(), child.HasChildren() });
                }

                return std::move(nodes);
            }
        );

        ImGui::BeginChild("HierarchyChild");
        const float nodeHeight = 14.f;
        m_Tree.OnImGuiRender(
            nodeHeight,
            Editor::GetState().SelectedEntity.IsValid() ? Editor::GetState().SelectedEntity.GetUUID() : Heart::UUID(0),
            [&](const VirtualizedTree::Node& node)
            {
                Heart::Entity entity = { &activeScene, (entt::entity)(uintptr_t)node.UserData };
                Heart::HStringView name = entity.GetName();

                // if dragging, we are going to be looking for a new parent to assign to
                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("EntityNode", &node.UserData, sizeof(entt::entity));
                    ImGui::Text("%s", name.DataUTF8());
                    ImGui::EndDragDropSource();
                }

                // if dropped on, set this entity as the new parent and expand
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
                    {
                        entt::entity payloadData = *(const entt::entity*)payload->Data;
                        activeScene.AssignRelationship(entity, { &activeScene, payloadData });

                        m_Tree.ExpandNode(node.Id);
                    }
                    ImGui::EndDragDropTarget();
                }

                // right click menu
                bool deleted = false;
                ImGui::PushID((void*)(uintptr_t)node.Id);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Create Child Entity"))
                    {
                        auto newEntity = Editor::GetActiveScene().CreateEntity("New Entity");
                        activeScene.AssignRelationship(entity, newEntity);
                        m_Tree.ExpandNode(node.Id);
                    }
                    if (ImGui::MenuItem("Remove Entity"))
                    {
                        activeScene.DestroyEntity(entity, true);
                        Editor::GetState().SelectedEntity = Heart::Entity();
                        deleted = true;
                    }
                    if (ImGui::MenuItem("Duplicate Entity"))
                    {
                        Editor::GetState().SelectedEntity = activeScene.DuplicateEntity(entity, true, true);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();

                ImGui::BeginHorizontal("helem", { -1.f, nodeHeight }, 0.5f);

                // Icons (TODO: more dynamic system)
                Heart::HStringView8 icon;
                if (entity.HasComponent<Heart::CameraComponent>())
                    icon = "editor/entity/camera.png";
                else if (entity.HasComponent<Heart::LightComponent>())
                    icon = "editor/entity/light.png";
                else if (entity.HasComponent<Heart::TextComponent>())
                    icon = "editor/entity/text.png";
                else if (entity.HasComponent<Heart::ScriptComponent>())
                    icon = "editor/entity/script.png";
                else if (entity.HasComponent<Heart::MeshComponent>())
                    icon = "editor/entity/cube.png";

                if (!icon.IsEmpty())
                {
                    ImGui::Image(
                        Heart::AssetManager::RetrieveAsset(Heart::HString8(icon), true)
                            ->EnsureValid<Heart::TextureAsset>()
                            ->GetTexture()
                            ->GetImGuiHandle(),
                        { nodeHeight, nodeHeight },
                        { 0, 0 }, { 1, 1 },
                        { 0.25f, 0.5f, 1.f, 1.f }
                    );
                }
                else
                    ImGui::Dummy({ nodeHeight, nodeHeight });

                ImGui::Text("%s", deleted ? "<Deleted>" : name.DataUTF8());
                ImGui::EndHorizontal();
            },
            [&activeScene](const VirtualizedTree::Node& node)
            {
                Editor::GetState().SelectedEntity = Heart::Entity(&activeScene, (entt::entity)(uintptr_t)node.UserData);
            }
        );
        ImGui::EndChild();

        // top level drag drop for parenting to root
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EntityNode"))
            {
                u32 payloadData = *(const u32*)payload->Data;
                Editor::GetActiveScene().UnparentEntity({ &Editor::GetActiveScene(), payloadData });
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("SceneHierarchyPopup", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Entity"))
                Editor::GetState().SelectedEntity = Editor::GetActiveScene().CreateEntity("New Entity");

            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
