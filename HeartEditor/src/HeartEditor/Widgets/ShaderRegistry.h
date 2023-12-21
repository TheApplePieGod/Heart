#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    class ShaderRegistry : public Widget
    {
    public:
        ShaderRegistry(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
        {}

        void OnImGuiRenderPostSceneUpdate() override;

    private:
        bool PassShaderTypeFilter(u32 type);

    private:
        ImGuiTextFilter m_PathFilter;
        u32 m_ShaderTypeFilter = 0;
    };
}
}
