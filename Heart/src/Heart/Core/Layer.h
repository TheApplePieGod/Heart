#pragma once

#include "Heart/Events/EventEmitter.h"
#include "Heart/Core/Timestep.h"

namespace Heart
{
    // TODO: describe method lifecycles
    class Layer : public EventListener
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @param name Debug name for the layer.
         */
        Layer(const std::string& name = "DefaultLayer");

        /*! @brief Default destructor. */
        virtual ~Layer() = default;

        /*! @brief Called when the layer is attached to the application. */
        virtual void OnAttach() {}

        /**
         * @brief Called each frame during the update step.
         *
         * @param ts The timestep for the previous frame.
         */
        virtual void OnUpdate(Timestep ts) {}

        /*! @brief Called each frame during the GUI render step. */
        virtual void OnImGuiRender() {}

        /*! @brief Called when the layer is detached from the application. */
        virtual void OnDetach() {}

        // virtual void OnEvent(Event& event) {};

    protected:
        std::string m_Name;
    };
}