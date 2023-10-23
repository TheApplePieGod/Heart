#pragma once

#include "Heart/Scripting/ScriptInstance.h"

namespace Heart
{
    class ScriptComponentInstance : public ScriptInstance
    {
    public:
        ScriptComponentInstance() = default;
        ~ScriptComponentInstance() = default;

        ScriptComponentInstance(s64 scriptClassId)
            : ScriptInstance(scriptClassId)
        {}

        void Instantiate();

        const ScriptClass& GetScriptClassObject() const override;

    private:

    };
}
