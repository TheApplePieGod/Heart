#pragma once

#include "Heart/Core/App.h"

namespace HeartEditor
{
    class EditorApp : public Heart::App
    {
    public:
        EditorApp();
        ~EditorApp() = default;

        void Close() override;

    private:
        
    };
}