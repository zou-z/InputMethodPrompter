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

export struct KeyEvent
{
    WPARAM Message;
    KBDLLHOOKSTRUCT Data;

    bool operator==(const KeyEvent& other) const noexcept
    {
        return Message == other.Message
            && Data.vkCode == other.Data.vkCode
            && Data.scanCode == other.Data.scanCode
            && Data.flags == other.Data.flags
            && Data.time == other.Data.time
            && Data.dwExtraInfo == other.Data.time;
    }
};

export struct PromptData
{
    std::shared_ptr<Observable<std::optional<TextBoxElement>>> FocusedTextBox;
    std::shared_ptr<Observable<std::optional<KeyEvent>>> KeyEvent;

    PromptData() :
        FocusedTextBox(std::make_shared<Observable<std::optional<TextBoxElement>>>(std::nullopt)),
        KeyEvent(std::make_shared<Observable<std::optional<::KeyEvent>>>(std::nullopt))
    {
    }
};