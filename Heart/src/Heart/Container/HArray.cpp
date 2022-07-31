#include "hepch.h"
#include "HArray.h"

namespace Heart
{
    void to_json(nlohmann::json& j, const HArray& array)
    {
        if (array.GetCount() == 0)
        {
            j = nlohmann::json::array();
            return;
        }

        for (u32 i = 0; i < array.GetCount(); i++)
            j[i] = array.Get(i);
    }

    void from_json(const nlohmann::json& j, HArray& array)
    {
        if (!j.is_array()) return;

        array.Resize(j.size(), false);
        for (auto it = j.begin(); it != j.end(); it++)
            array.Add(it.value());
    }
}