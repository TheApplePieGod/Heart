#include "hepch.h"
#include "Viewport.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Core/Window.h"
#include "Heart/Scene/Scene.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui_internal.h"
#include "glm/vec4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace HeartEditor
{
namespace Widgets
{
    Viewport::Viewport(const std::string& name, bool initialOpen)
        : Widget(name, initialOpen)
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
        m_EditorCamera = Heart::CreateScope<EditorCamera>(70.f, 0.1f, 500.f, 1.f, glm::vec3(0.f, 1.f, 0.f));
    }

    void Viewport::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::Begin(m_Name.c_str(), &m_Open);

        m_EditorCamera->OnUpdate(EditorApp::Get().GetLastTimestep(), m_ViewportInput, m_ViewportHover);

        m_SceneRenderer->RenderScene(
            EditorApp::Get().GetWindow().GetContext(),
            &Editor::GetActiveScene(),
            *m_EditorCamera,
            m_EditorCamera->GetPosition(),
            true
        );
        
        // calculate viewport bounds & aspect ratio
        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        m_ViewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
        glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
        glm::vec2 viewportEnd = viewportStart + m_ViewportSize;
        m_EditorCamera->UpdateAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
        m_ViewportMousePos = glm::vec2(std::clamp(ImGui::GetMousePos().x - viewportStart.x, 0.f, m_ViewportSize.x), std::clamp(ImGui::GetMousePos().y - viewportStart.y, 0.f, m_ViewportSize.y));

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background

        // initialize imguizmo & draw the grid
        glm::mat4 view = m_EditorCamera->GetViewMatrixInvertedY();
        glm::mat4 proj = m_EditorCamera->GetProjectionMatrix();
        glm::mat4 identity(1.f);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportStart.x, viewportStart.y, m_ViewportSize.x, m_ViewportSize.y);
        //ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(identity), 100.f);

        // draw the rendered texture
        ImGui::Image(
            m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
            { m_ViewportSize.x, m_ViewportSize.y }
        );

        // enable input if the viewport is being right clicked
        m_ViewportHover = ImGui::IsItemHovered();
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
        {
            // disable imgui input & cursor
            m_ViewportInput = true;
            EditorApp::Get().GetWindow().DisableCursor();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            ImGui::SetWindowFocus();
        }

        // gizmo buttons
        ImGui::SetCursorPos(viewportMin);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 0.f));
        if (ImGui::BeginTable("GizmoOperations", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();

            // translate
            ImGui::TableSetColumnIndex(0);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/pan.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE; }
            Heart::ImGuiUtils::RenderTooltip("Change gizmo to translate mode");

            // rotate
            ImGui::TableSetColumnIndex(1);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/rotate.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::ROTATE; }
            Heart::ImGuiUtils::RenderTooltip("Change gizmo to rotate mode");

            // scale
            ImGui::TableSetColumnIndex(2);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>("editor/scale.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoOperation = ImGuizmo::OPERATION::SCALE; }
            Heart::ImGuiUtils::RenderTooltip("Change gizmo to scale mode");

            ImGui::TableSetColumnIndex(3);
            ImGui::Dummy({ 15.f, 0.f });

            // world/object space
            ImGui::TableSetColumnIndex(4);
            if (ImGui::ImageButton(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(m_GizmoMode ? "editor/world.png" : "editor/object.png", true)->GetTexture()->GetImGuiHandle(),
                { 25, 25 }
            ))
            { m_GizmoMode = (ImGuizmo::MODE)(!m_GizmoMode); }
            Heart::ImGuiUtils::RenderTooltip(m_GizmoMode ? "Change gizmo to operate in local space" : "Change gizmo to operate in world space");

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // hover is false if we are hovering over the buttons
        m_ViewportHover = m_ViewportHover && !ImGui::IsItemHovered();

        Heart::ImGuiUtils::AssetDropTarget(
            Heart::Asset::Type::Scene,
            [](const std::string& path) { Editor::SetActiveScene(Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, path))->GetScene()); }
        );

        // draw the imguizmo if an entity is selected
        auto selectedEntity = Editor::GetState().SelectedEntity;
        if (selectedEntity.IsValid())
        {
            glm::mat4 parentTransform;
            glm::mat4 transform = Editor::GetActiveScene().CalculateEntityTransform(selectedEntity, &parentTransform);

            ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(proj),
                m_GizmoOperation,
                m_GizmoMode,
                glm::value_ptr(transform),
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 invscale, invtranslation, invskew;
                glm::vec4 invperspective;
                glm::quat invrotation;
            
                // convert the word space transform to local space by multiplying it by the inverse of the parent transform
                glm::decompose(glm::inverse(parentTransform) * transform, invscale, invrotation, invtranslation, invskew, invperspective);
                selectedEntity.SetTransform(invtranslation, glm::degrees(glm::eulerAngles(invrotation)), invscale);
            }
        }

        ImGui::End();
    }
}
}