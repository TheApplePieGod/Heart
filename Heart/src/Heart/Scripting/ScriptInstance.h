#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Container/HString.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    class Variant;
    class Scene;
    class Entity;
    class ScriptClass;
    class ScriptInstance
    {
    public:
        ScriptInstance() = default;
        ~ScriptInstance() = default;

        ScriptInstance(s64 scriptClassId)
            : m_ScriptClassId(scriptClassId)
        {}

        // Instantiate functionality exists per-subclass

        void ConsumeHandle(uptr newHandle);
        void Destroy();
        void Clear();

        Variant GetFieldValue(const HStringView& fieldName) const;
        bool SetFieldValue(const HStringView& fieldName, const Variant& value, bool invokeCallback);

        nlohmann::json SerializeFieldsToJson();
        void* SerializeFieldsToBinary();
        void LoadFieldsFromJson(const nlohmann::json& j);
        void LoadFieldsFromBinary(void* data);
        
        bool IsInstantiable();

        // Must return default script class on failure
        virtual const ScriptClass& GetScriptClassObject() const = 0;

        inline void ClearObjectHandle() { m_ObjectHandle = 0; }
        inline uptr GetObjectHandle() const { return m_ObjectHandle; }
        inline s64 GetScriptClassId() const { return m_ScriptClassId; }
        inline void SetScriptClassId(s64 newId) { m_ScriptClassId = newId; }
        inline bool HasScriptClass() const { return m_ScriptClassId != 0; }
        inline bool IsAlive() const { return m_ObjectHandle != 0; }

    protected:
        Variant GetFieldValueUnchecked(const HStringView& fieldName) const;
        bool SetFieldValueUnchecked(const HStringView& fieldName, const Variant& value, bool invokeCallback);

    protected:
        uptr m_ObjectHandle = 0;
        s64 m_ScriptClassId alignas(8);
    };
}
