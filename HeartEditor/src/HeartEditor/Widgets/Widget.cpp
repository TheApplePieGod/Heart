#include "hepch.h"
#include "Widget.h"

namespace HeartEditor
{
    nlohmann::json Widget::Serialize()
    {
        nlohmann::json j;
        j["open"] = m_Open;

        return j;
    }

    void Widget::Deserialize(const nlohmann::json& elem)
    {
        if (elem.contains("open"))
            m_Open = elem["open"];
    }
}