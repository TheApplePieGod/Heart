#pragma once

#include "Heart/Scripting/ScriptInstance.h"

namespace Heart
{
    class Timestep;
    class Entity;
    class ScriptEntityInstance : public ScriptInstance
    {
    public:
        ScriptEntityInstance() = default;
        ~ScriptEntityInstance() = default;

        ScriptEntityInstance(s64 scriptClassId)
            : ScriptInstance(scriptClassId)
        {}

        void Instantiate(Entity owner);

        void OnConstruct();
        void OnPlayStart();
        void OnPlayEnd();
        void OnUpdate(Timestep ts);
        void OnCollisionStarted(Entity other);
        void OnCollisionEnded(Entity other);

        const ScriptClass& GetScriptClassObject() const override;

    private:

    };
}
