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

        ScriptInstance(const HStringView& scriptClass)
            : m_ScriptClass(scriptClass)
        {}

        void Instantiate(Entity owner);
        void Destroy();
        void Clear();

        void OnConstruct();
        void OnPlayStart();
        void OnPlayEnd();
        void OnUpdate(Timestep ts);
        void OnCollisionStarted(Entity other);
        void OnCollisionEnded(Entity other);

        Variant GetFieldValue(const HStringView& fieldName) const;
        bool SetFieldValue(const HStringView& fieldName, const Variant& value, bool invokeCallback);

        nlohmann::json SerializeFieldsToJson();
        void* SerializeFieldsToBinary();
        void LoadFieldsFromJson(const nlohmann::json& j);
        void LoadFieldsFromBinary(void* data);
        
        bool ValidateClass();

        inline void ClearObjectHandle() { m_ObjectHandle = 0; }
        inline uptr GetObjectHandle() const { return m_ObjectHandle; }
        inline const HString& GetScriptClass() const { return m_ScriptClass; }
        inline void SetScriptClass(const HStringView& value) { m_ScriptClass = value; }
        inline bool IsInstantiable() const { return !m_ScriptClass.IsEmpty(); }
        inline bool IsAlive() const { return m_ObjectHandle != 0; }

    private:
        Variant GetFieldValueUnchecked(const HStringView& fieldName) const;
        bool SetFieldValueUnchecked(const HStringView& fieldName, const Variant& value, bool invokeCallback);

    private:
        uptr m_ObjectHandle = 0;
        HString m_ScriptClass alignas(8);
    };
}
