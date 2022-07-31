using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Heart.Scene
{
    public abstract class Entity
    {
        public virtual void OnPlayStart() {}
        public virtual void OnPlayEnd() {}
        public virtual void OnUpdate(float timestep) {}
    }
}
