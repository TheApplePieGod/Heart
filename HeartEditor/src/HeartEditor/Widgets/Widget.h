#pragma once

#include "nlohmann/json.hpp"
#include "Heart/Container/HString.h"

namespace HeartEditor
{
    class Widget
    {
    public:
        Widget(const Heart::HStringView& name, bool initialOpen)
            : m_Name(name), m_Open(initialOpen)
        {}

        virtual void OnImGuiRender() = 0;
        virtual nlohmann::json Serialize();
        virtual void Deserialize(const nlohmann::json& elem);

        inline const Heart::HString& GetName() const { return m_Name; }
        inline bool IsOpen() const { return m_Open; }
        inline bool IsDirty() const { return m_Dirty; }
        inline void SetOpen(bool open) { m_Open = open; }
        inline void SetDirty(bool dirty) { m_Dirty = dirty; }

    protected:
        Heart::HString m_Name;
        bool m_Open;
        bool m_Dirty = false;
    };
}