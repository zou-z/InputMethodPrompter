module;
#include <Windows.h>
export module TrayIconV2;

import std;
import StringsV2;
import RegistryService;
import LogComponent;

export class TrayIcon
{
public:
    TrayIcon(std::shared_ptr<ILog> log, const std::wstring& applicationName) :
        log(std::move(log)),
        applicationName(applicationName),
        notifyIconData(),
        registryKey(LR"(Software\Microsoft\Windows\CurrentVersion\Run)")
    {
    }

    static std::expected<std::unique_ptr<TrayIcon>, DWORD> Create(
        HINSTANCE instance,
        std::shared_ptr<ILog> log,
        const std::wstring& applicationName)
    {
        constexpr auto className = L"TrayMessageWindow";
        constexpr int IDI_APPICON = 1;
        constexpr int trayIconId = 1;

        WNDCLASSEXW windowClass =
        {
            .cbSize = sizeof(WNDCLASSEXW),
            .lpfnWndProc = TrayWindowProcedure,
            .hInstance = instance,
            .lpszClassName = className,
        };
        if (RegisterClassExW(&windowClass) == 0)
            return std::unexpected(GetLastError());

        auto trayIcon = std::make_unique<TrayIcon>(std::move(log), applicationName);
        auto parameter = static_cast<void*>(trayIcon.get());
        auto windowHandle = CreateWindowW(className, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, parameter);
        if (windowHandle == nullptr)
            return std::unexpected(GetLastError());

        trayIcon->notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
        trayIcon->notifyIconData.hWnd = windowHandle;
        trayIcon->notifyIconData.uID = trayIconId;
        trayIcon->notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        trayIcon->notifyIconData.uCallbackMessage = trayCallbackMessage;
        trayIcon->notifyIconData.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON));
        wcscpy_s(trayIcon->notifyIconData.szTip, applicationName.data());

        return trayIcon;
    }

    bool Show()
    {
        return Shell_NotifyIcon(NIM_ADD, &notifyIconData) == TRUE;
    }

    bool Close()
    {
        return Shell_NotifyIcon(NIM_DELETE, &notifyIconData) == TRUE;
    }

private:
    void ShowPopupMenu(HWND handle)
    {
        POINT point;
        GetCursorPos(&point);

        auto isAutoStart = registryKey.HasSubKey(applicationName) == ERROR_SUCCESS;

        HMENU menu = CreatePopupMenu();
        auto autoStartFlags = MF_STRING | (isAutoStart ? MF_CHECKED : MF_UNCHECKED);
        AppendMenu(menu, autoStartFlags, autoStartTrayId, Strings::AutoStart.data());
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING, aboutTrayId, Strings::About.data());
        AppendMenu(menu, MF_STRING, exitTrayId, Strings::Exit.data());

        SetForegroundWindow(handle);
        TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY, point.x, point.y, 0, handle, nullptr);
        PostMessage(handle, WM_NULL, 0, 0);
        DestroyMenu(menu);
    }

    void ShowAbout(HWND handle)
    {
        //auto message =
        //    Strings::ApplicationName.data() + std::wstring(L"\r\n") +
        //    std::wstring(L"v") + Strings::ApplicationVersion.data() + std::wstring(L"\r\n") +
        //    Strings::ApplicationLink.data();

        //messageBox.Show(handle, message.c_str(), Strings::About.data(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    }

    void SwitchAutoStart()
    {
        auto isAutoStart = registryKey.HasSubKey(applicationName) == ERROR_SUCCESS;
        if (isAutoStart)
        {
            registryKey.RemoveSubKey(applicationName);
        }
        else
        {
            WCHAR path[MAX_PATH];
            GetModuleFileName(nullptr, path, MAX_PATH);
            registryKey.AddSubKeyValue(applicationName, path);
        }

        Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
    }

    LRESULT OnTrayWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case trayCallbackMessage:
        {
            if (lParam == WM_RBUTTONUP)
                ShowPopupMenu(hWnd);
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case autoStartTrayId: SwitchAutoStart(); break;
            case aboutTrayId: ShowAbout(hWnd); break;
            case exitTrayId: DestroyWindow(hWnd); return 0;
            }
            break;
        }

        case WM_DESTROY:
            Close();
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    static LRESULT CALLBACK TrayWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        TrayIcon* self = nullptr;

        if (message == WM_NCCREATE)
        {
            auto create = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = static_cast<TrayIcon*>(create->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        }
        else
        {
            self = reinterpret_cast<TrayIcon*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        return self == nullptr
            ? DefWindowProc(hWnd, message, wParam, lParam)
            : self->OnTrayWindowProcedure(hWnd, message, wParam, lParam);
    }

private:
    static constexpr UINT trayCallbackMessage = WM_USER + 1;
    static constexpr UINT autoStartTrayId = 1003;
    static constexpr UINT aboutTrayId = 1002;
    static constexpr UINT exitTrayId = 1001;
    std::shared_ptr<ILog> log;
    std::wstring applicationName;
    NOTIFYICONDATAW notifyIconData;
    RegistryKey registryKey;
};