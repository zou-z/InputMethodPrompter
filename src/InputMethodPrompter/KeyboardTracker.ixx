module;
#include <Windows.h>
export module KeyboardTracker;

import std;
import PromptData;
import LogComponent;
import ReactiveComponent;

using namespace Reactive;

class KeyboardHookV2
{
public:
    KeyboardHookV2() = delete;

    static DWORD Install(HINSTANCE instance, std::function<void(WPARAM, LPARAM)> onKeyEvent)
    {
        if (hook == nullptr)
        {
            hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProcedure, instance, 0);
            if (hook == nullptr)
                return GetLastError();
        }

        KeyboardHookV2::onKeyEvent = onKeyEvent;

        return ERROR_SUCCESS;
    }

    static DWORD Uninstall()
    {
        if (hook != nullptr && !UnhookWindowsHookEx(hook))
            return GetLastError();

        hook = nullptr;
        KeyboardHookV2::onKeyEvent = nullptr;

        return ERROR_SUCCESS;
    }

private:
    static LRESULT CALLBACK KeyboardProcedure(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code == HC_ACTION)
        {
            if (onKeyEvent != nullptr)
                onKeyEvent(wParam, lParam);
        }

        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

private:
    inline static HHOOK hook = nullptr;
    inline static std::function<void(WPARAM, LPARAM)> onKeyEvent;
};

export class KeyboardTracker
{
public:
    KeyboardTracker(
        std::shared_ptr<ILog> log,
        std::weak_ptr<IPublisher<DWORD>> releasedVirtualKeyCode) :
        log(std::move(log)),
        releasedVirtualKeyCode(releasedVirtualKeyCode)
    {
    }

    DWORD Start(HINSTANCE instance)
    {
        return KeyboardHookV2::Install(instance, [this](WPARAM wParam, LPARAM lParam)
        {
            OnKeyEvent(wParam, lParam);
        });
    }

    DWORD Stop()
    {
        return KeyboardHookV2::Uninstall();
    }

private:
    void OnKeyEvent(WPARAM wParam, LPARAM lParam)
    {
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            if (auto virtualKeyCode = releasedVirtualKeyCode.lock())
            {
                auto keyboardStruct = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
                if (!virtualKeyCode->Set(keyboardStruct->vkCode))
                    log->Error("Failed to set key event");
            }
        }
    }

private:
    std::shared_ptr<ILog> log;
    std::weak_ptr<IPublisher<DWORD>> releasedVirtualKeyCode;
};