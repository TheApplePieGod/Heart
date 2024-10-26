#pragma once

#include "Heart/Core/App.h"

namespace HeartEditor
{
    class EditorApp : public Heart::App
    {
    public:
        EditorApp();
        ~EditorApp();

        void StartEditor(Heart::HStringView8 windowName);

        void Close() override;

    private:
        
    };
}
