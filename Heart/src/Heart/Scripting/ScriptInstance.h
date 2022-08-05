#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Container/HString.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    class Timestep;
    class Variant;
    class Scene;
    class Entity;
    class ScriptInstance
    {
    public:
        ScriptInstance() = default;
        ~ScriptInstance() = default;

        ScriptInstance(const HString& scriptClass)
            : m_ScriptClass(scriptClass)
        {}

        void Instantiate(Entity owner);
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
        
        bool ValidateClass();

        inline void ClearObjectHandle() { m_ObjectHandle = 0; }
        inline uptr GetObjectHandle() const { return m_ObjectHandle; }
        inline const HString& GetScriptClass() const { return m_ScriptClass; }
        inline void SetScriptClass(const HString& value) { m_ScriptClass = value; }
        inline bool IsInstantiable() const { return !m_ScriptClass.IsEmpty(); }
        inline bool IsAlive() const { return m_ObjectHandle != 0; }

    private:
        Variant GetFieldValueUnchecked(const HString& fieldName) const;
        bool SetFieldValueUnchecked(const HString& fieldName, const Variant& value);

    private:
        uptr m_ObjectHandle = 0;
        HString m_ScriptClass alignas(8);
    };
}