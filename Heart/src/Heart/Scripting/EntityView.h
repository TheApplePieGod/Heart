#pragma once

#include "entt/entt.hpp"

namespace Heart
{
    struct EntityView
    {
        entt::runtime_view View{};
        entt::runtime_view::iterator Current;
    };
}
