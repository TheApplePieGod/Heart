#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Container/HString.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    class Timestep;
    class Variant;
    class ScriptInstance
    {
    public:
        ScriptInstance() = default;
        ~ScriptInstance()
        {
            if (IsAlive())
                HE_ENGINE_LOG_WARN("Script instance destructor");
        }

        ScriptInstance(const HString& scriptClass)
            : m_ScriptClass(scriptClass)
        {}

        void Instantiate();
        void Destroy();
        void Clear();

        void OnPlayStart();
        void OnPlayEnd();
        void OnUpdate(Timestep ts);

        Variant GetFieldValue(const HString& fieldName) const;
        bool SetFieldValue(const HString& fieldName, const Variant& value);

        nlohmann::json SerializeFieldsToJson();
        void* SerializeFieldsToBinary();
        void LoadFieldsFromJson(const nlohmann::json& j);
        void LoadFieldsFromBinary(void* data);

        inline uptr GetObjectHandle() const { return m_ObjectHandle; }
        inline const HString& GetScriptClass() const { return m_ScriptClass; }
        inline bool IsInstantiable() const { return !m_ScriptClass.Empty(); }
        inline bool IsAlive() const { return m_ObjectHandle != 0; }

    private:
        Variant GetFieldValueUnchecked(const HString& fieldName) const;
        bool SetFieldValueUnchecked(const HString& fieldName, const Variant& value);

    private:
        uptr m_ObjectHandle = 0;
        HString m_ScriptClass;
    };
}