#pragma once

#include "Heart/Core/UUID.h"
#include "Heart/Container/HString.h"

namespace Heart
{
    class Timestep;
    class Variant;
    class ScriptInstance
    {
    public:
        ScriptInstance() = default;

        ScriptInstance(const HString& scriptClass)
            : m_ScriptClass(scriptClass)
        {}

        void Instantiate();
        void Destroy();
        void Clear();

        void OnPlayStart();
        void OnPlayEnd();
        void OnUpdate(Timestep ts);

        Variant GetFieldValue(const HString& fieldName);
        bool SetFieldValue(const HString& fieldName, const Variant& value);

        inline uptr GetObjectHandle() const { return m_ObjectHandle; }
        inline const HString& GetScriptClass() const { return m_ScriptClass; }
        inline bool IsInstantiable() const { return !m_ScriptClass.Empty(); }
        inline bool IsAlive() const { return m_ObjectHandle != 0; }

    private:
        uptr m_ObjectHandle = 0;
        HString m_ScriptClass;
    };
}