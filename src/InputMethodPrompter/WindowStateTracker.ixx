module;
#include <Windows.h>
export module WindowStateTracker;

import std;
import LogComponent;
import ReactiveComponent;

using namespace Reactive;

class WindowEventHook
{
public:
    WindowEventHook() = delete;

    static DWORD Install(std::function<void(DWORD, HWND)> onEvent)
    {
        if (hook != nullptr)
        {
            auto errorCode = Uninstall();
            if (errorCode != ERROR_SUCCESS)
                return errorCode;
        }

        hook = SetWinEventHook(
            EVENT_SYSTEM_MOVESIZESTART,
            EVENT_OBJECT_LOCATIONCHANGE,
            nullptr,
            WindowEventProcedure,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );

        if (hook == nullptr)
            return GetLastError();

        WindowEventHook::onEvent = onEvent;

        return ERROR_SUCCESS;
    }

    static DWORD Uninstall()
    {
        if (hook != nullptr && !UnhookWinEvent(hook))
            return GetLastError();

        hook = nullptr;
        onEvent = nullptr;

        return ERROR_SUCCESS;
    }

private:
    static void CALLBACK WindowEventProcedure(
       HWINEVENTHOOK hWinEventHook,
       DWORD event,
       HWND hwnd,
       LONG idObject,
       LONG idChild,
       DWORD idEventThread,
       DWORD dwmsEventTime)
    {
        if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
            return;

        switch (event)
        {
        case EVENT_OBJECT_LOCATIONCHANGE:
        case EVENT_SYSTEM_MOVESIZEEND:
            onEvent(event, hwnd);
            break;
        }
    }

private:
    inline static HWINEVENTHOOK hook = nullptr;
    inline static std::function<void(DWORD, HWND)> onEvent;
};

export class WindowStateTracker
{
public:
    WindowStateTracker(
        std::shared_ptr<ILog> log,
        std::weak_ptr<IPublisher<HWND>> eventWindow) :
        log(std::move(log)),
        eventWindow(eventWindow)
    {
    }

    ~WindowStateTracker()
    {
        WindowEventHook::Uninstall();
    }

    DWORD Start()
    {
        return WindowEventHook::Install([this](DWORD eventType, HWND windowHandle)
        {
            OnWindowEvent(eventType, windowHandle);
        });
    }

    DWORD Stop()
    {
        return WindowEventHook::Uninstall();
    }

private:
    void OnWindowEvent(DWORD eventType, HWND windowHandle)
    {
        if (auto window = eventWindow.lock())
        {
            if (!window->Set(windowHandle))
                log->Error("Failed to set event window handle");
        }
    }

private:
    std::shared_ptr<ILog> log;
    std::weak_ptr<IPublisher<HWND>> eventWindow;
};