using Heart.Core;
using System.Runtime.CompilerServices;

namespace Heart.Scene
{
    public abstract class Entity
    {
        public virtual void OnPlayStart() {}

        public virtual void OnPlayEnd() {}

        protected void OnUpdate_Internal(double timestep) { OnUpdate(new Timestep(timestep)); }
        public virtual void OnUpdate(Timestep timestep) {}
    }
}
