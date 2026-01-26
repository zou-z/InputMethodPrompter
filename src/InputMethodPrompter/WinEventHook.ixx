export module WinEventHook;

import std;
import <Windows.h>;

export class WinEventHook
{
public:
    WinEventHook() = delete;

    static DWORD Install(std::function<void(HWND)> onWinEvent)
    {
        if (hook == nullptr)
        {
            hook = SetWinEventHook(
                EVENT_SYSTEM_MOVESIZESTART,
                EVENT_OBJECT_LOCATIONCHANGE,
                nullptr,
                WinEventProcedure,
                0,
                0,
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
            );

            if (hook == nullptr)
                return GetLastError();
        }

        WinEventHook::onWinEvent = onWinEvent;

        return S_OK;
    }

    static DWORD Uninstall()
    {
        if (hook != nullptr && FAILED(UnhookWinEvent(hook)))
            return GetLastError();

        hook = nullptr;
        onWinEvent = nullptr;

        return S_OK;
    }

private:
    static void CALLBACK WinEventProcedure(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD idEventThread,
        DWORD dwmsEventTime)
    {
        if (idObject != OBJID_WINDOW)
            return;

        switch (event)
        {
        case EVENT_OBJECT_LOCATIONCHANGE:
        case EVENT_SYSTEM_MOVESIZEEND:
            onWinEvent(hwnd);
            break;
        }
    }

private:
    inline static HWINEVENTHOOK hook = nullptr;
    inline static std::function<void(HWND)> onWinEvent;
};