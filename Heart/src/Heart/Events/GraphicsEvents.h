#pragma once

#include "Heart/Events/Event.h"
#include "Heart/Asset/Asset.h"

namespace Heart
{
    class TextureDeletedEvent : public Event
    {
    public:
        TextureDeletedEvent(void* deleting)
            : Event(EventType::TextureDeleted), m_Deleting(deleting)
        {}
        inline void* GetDeletingPointer() const { return m_Deleting; }

    public:
        inline static EventType GetStaticType() { return EventType::TextureDeleted; }

    private:
        void* m_Deleting;
    };
}