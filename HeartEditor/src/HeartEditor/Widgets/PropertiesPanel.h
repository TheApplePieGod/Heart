#pragma once

#include "Heart/Scene/Scene.h"
#include "Heart/Scene/Entity.h"

namespace HeartEditor
{
namespace Widgets
{
    class PropertiesPanel
    {
    public:
        PropertiesPanel();

        void OnImGuiRender(Heart::Entity selectedEntity);

    private:
        void RenderXYZSlider(const std::string& name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step);
    };
}
}