export module TrayIcon;

import std;
import Strings;
import SingletonMessageBox;
import RegistryKey;
import <Windows.h>;

export class TrayIcon
{
public:
    TrayIcon() :
        notifyIconData(),
        registryKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run")
    {
    }

    bool Init(HINSTANCE instance)
    {
        auto trayWindowClassName = L"TrayMessageWindow";

        if (RegisterMessageWindowClass(instance, trayWindowClassName) == 0)
            return false;

        auto windowHandle = CreateMessageWindow(instance, trayWindowClassName);
        if (windowHandle == nullptr)
            return false;

        InitNotifyIconData(instance, windowHandle);
        return Shell_NotifyIcon(NIM_ADD, &notifyIconData) == TRUE;
    }

private:
    void InitNotifyIconData(HINSTANCE instance, HWND windowHandle)
    {
        notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
        notifyIconData.hWnd = windowHandle;
        notifyIconData.uID = trayIconId;
        notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        notifyIconData.uCallbackMessage = trayCallbackMessage;
        notifyIconData.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON));
        wcscpy_s(notifyIconData.szTip, Strings::ApplicationName.data());
    }

    ATOM RegisterMessageWindowClass(HINSTANCE instance, const std::wstring& className)
    {
        WNDCLASSEXW windowClass =
        {
            .cbSize = sizeof(WNDCLASSEXW),
            .lpfnWndProc = TrayWindowProcedure,
            .hInstance = instance,
            .lpszClassName = className.c_str(),
        };

        return RegisterClassExW(&windowClass);
    }

    HWND CreateMessageWindow(HINSTANCE instance, const std::wstring& className)
    {
        return CreateWindowW(className.c_str(), L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, this);
    }

    void ShowPopupMenu(HWND handle)
    {
        POINT point;
        GetCursorPos(&point);

        HMENU menu = CreatePopupMenu();

        auto isAutoStart = registryKey.HasKey(Strings::ApplicationName.data()) == ERROR_SUCCESS;
        AppendMenu(
            menu,
            MF_STRING | (isAutoStart ? MF_CHECKED : MF_UNCHECKED),
            trayAutoStart,
            Strings::ApplicationAutoStart.data()
        );

        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenu(menu, MF_STRING, trayAboutId, Strings::About.data());
        AppendMenu(menu, MF_STRING, trayExitId, Strings::Exit.data());

        SetForegroundWindow(handle);

        TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY, point.x, point.y, 0, handle, nullptr);

        PostMessage(handle, WM_NULL, 0, 0);

        DestroyMenu(menu);
    }

    void ShowAbout(HWND handle)
    {
        auto message =
            Strings::ApplicationName.data() + std::wstring(L"\r\n") +
            std::wstring(L"v") + Strings::ApplicationVersion.data() + std::wstring(L"\r\n") +
            Strings::ApplicationLink.data();

        messageBox.Show(handle, message.c_str(), Strings::About.data(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
    }

    void SwitchAutoStart()
    {
        auto isAutoStart = registryKey.HasKey(Strings::ApplicationName.data()) == ERROR_SUCCESS;
        if (isAutoStart)
        {
            registryKey.RemoveKey(Strings::ApplicationName.data());
        }
        else
        {
            WCHAR path[MAX_PATH];
            GetModuleFileName(nullptr, path, MAX_PATH);
            registryKey.AddKeyValue(Strings::ApplicationName.data(), path);
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
        }
        break;

        case WM_COMMAND:
        {
            auto id = LOWORD(wParam);
            if (id == trayExitId)
            {
                PostQuitMessage(0);
            }
            else if (id == trayAboutId)
            {
                ShowAbout(hWnd);
            }
            else if (id == trayAutoStart)
            {
                SwitchAutoStart();
            }
        }
        break;

        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
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
            auto create = (CREATESTRUCT*)lParam;
            self = (TrayIcon*)create->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        }
        else
        {
            self = (TrayIcon*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        return self == nullptr
            ? DefWindowProc(hWnd, message, wParam, lParam)
            : self->OnTrayWindowProcedure(hWnd, message, wParam, lParam);
    }

private:
    static constexpr int IDI_APPICON = 1;
    static constexpr int trayIconId = 1;
    static constexpr UINT trayCallbackMessage = WM_USER + 1;
    static constexpr UINT trayExitId = 1001;
    static constexpr UINT trayAboutId = 1002;
    static constexpr UINT trayAutoStart = 1003;

    SingletonMessageBox messageBox;
    NOTIFYICONDATAW notifyIconData;
    RegistryKey registryKey;
};