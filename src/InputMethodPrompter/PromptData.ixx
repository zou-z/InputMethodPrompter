module;
#include <Windows.h>
#include <winrt/base.h>
#include <UIAutomation.h>
export module PromptData;

import std;
import ReactiveComponent;
import TextBoxService;

using namespace Reactive;

export struct TextBoxElement
{
    winrt::com_ptr<IUIAutomationElement> Value;
    TextBoxType Type;
    HWND WindowHandle;

    bool operator==(const TextBoxElement& other) const noexcept
    {
        return Value.get() == other.Value.get() && Type == other.Type && WindowHandle == other.WindowHandle;
    }
};

export struct PromptData
{
    std::shared_ptr<Observable<std::optional<TextBoxElement>>> FocusedTextBox;
    std::shared_ptr<Observable<DWORD>> ReleasedVirtualKeyCode;
    std::shared_ptr<Observable<HWND>> EventWindow;

    PromptData() :
        FocusedTextBox(std::make_shared<Observable<std::optional<TextBoxElement>>>(std::nullopt)),
        ReleasedVirtualKeyCode(std::make_shared<Observable<DWORD>>(0, true)),
        EventWindow(std::make_shared<Observable<HWND>>(nullptr, true))
    {
    }
};