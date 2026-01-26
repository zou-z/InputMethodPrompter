export module KeyboardHook;

import std;
import <Windows.h>;

export class KeyboardHook
{
public:
    KeyboardHook() = delete;

    static DWORD Install(HINSTANCE instance, std::function<void(WPARAM, LPARAM)> onKeyEvent)
    {
        if (hook == nullptr)
        {
            hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProcedure, instance, 0);
            if (hook == nullptr)
                return GetLastError();
        }

        KeyboardHook::onKeyEvent = onKeyEvent;

        return S_OK;
    }

    static DWORD Uninstall()
    {
        if (hook != nullptr && FAILED(UnhookWindowsHookEx(hook)))
            return GetLastError();

        hook = nullptr;
        KeyboardHook::onKeyEvent = nullptr;

        return S_OK;
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