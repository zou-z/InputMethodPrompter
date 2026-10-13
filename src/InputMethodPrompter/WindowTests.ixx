module;
#include <Windows.h>
#include "GraphicsTypes.h"
#include "Macros.h"
export module WindowTests;

import std;
import Strings;
import WindowComponent;
import PromptWindowV2;


export class WindowTests
{
public:
    WindowTests()
    {
    }

    int RunTests(HINSTANCE instance)
    {
        auto result = Window::Create<PromptWindow>(
            instance,
            Strings::ApplicationName.data(),
            Strings::ApplicationName.data() + std::wstring(L" Class")
        );
        if (!result.has_value())
            return 1;

        std::unique_ptr<PromptWindow> window = std::move(result.value());
        //window->GetBounds().SetPosition(50, 50);
        window->GetBounds().SetPosition(150, 500);
        window->GetState().Show();
        window->GetState().SetTopmost(true);
        window->SetInputMethodState(InputMethodState::ChineseInputMode);
        //window->SetIsNumLocked(true);
        window->SetIsCapsLocked(true);
        //window->SetIsScrollLocked(true);

        MSG message;
        while (GetMessage(&message, nullptr, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return static_cast<int>(message.wParam);
    }
};