#pragma once

#include "Heart/Container/HString.h"

namespace Heart
{
    class ScriptClass
    {
    public:

        void LoadMethods();

    private:
        HString m_FullName;
        
    };
}