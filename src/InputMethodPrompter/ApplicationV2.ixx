module;
#include <Windows.h>
#include <Shlwapi.h>
#include "Macros.h"
export module ApplicationV2;

import std;
import Strings;
import PromptWindowV2;
import PromptData;
import LogComponent;
import EnumComponent;
import AutomationComponent;
import FocusedTextBoxTracker;
import TextBoxService;
import KeyboardTracker;
import WindowStateTracker;
import TrayIconV2;

class SingletonMutex
{
public:
    ~SingletonMutex()
    {
        if (mutex != nullptr)
        {
            ReleaseMutex(mutex);
            CloseHandle(mutex);
            mutex = nullptr;
        }
    }

    DWORD Create(const std::wstring& name)
    {
        RETURN_IF(mutex != nullptr, S_OK);
        mutex = CreateMutex(nullptr, TRUE, name.c_str());
        return GetLastError();
    }

private:
    HANDLE mutex = nullptr;
};

export class Application
{
public:
    Application()
    {
    }

    int Run(HINSTANCE instance)
    {
        SingletonMutex mutex;
        RETURN_IF(!EnsureSingleton(mutex), 0);

        if (!Initialize(instance))
            return 0;

        MSG message;
        while (GetMessage(&message, nullptr, 0, 0))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        return static_cast<int>(message.wParam);
    }

private:
    bool EnsureSingleton(SingletonMutex& mutex)
    {
        auto result = mutex.Create(L"InputMethodPrompter_SingletonMutex");
        RETURN_IF(result == ERROR_SUCCESS, true);

        MessageBox(
            nullptr,
            result == ERROR_ALREADY_EXISTS
                ? Strings::ApplicationAlreadyRunning.data()
                : (L"Failed to create mutex, code: " + std::to_wstring(result)).c_str(),
            Strings::ApplicationName.data(),
            MB_OK | MB_ICONEXCLAMATION
        );

        return false;
    }

    bool Initialize(HINSTANCE instance)
    {
        auto fullPath = GetApplicationPath(instance);
        auto path = GetFolderPath(fullPath);
        auto name = GetFileNameWithoutExtension(fullPath);

        log = std::make_shared<Log>(path, name, Log::LogLevel::Info);
        textBoxService = std::make_shared<TextBoxService>();

        auto createAutomationResult = Automation::Create();
        if (!createAutomationResult.has_value())
        {
            FastLog::HRESULT::Fatal(log, "Failed to create automation", createAutomationResult.error());
            return false;
        }
        automation = std::move(createAutomationResult.value());

        auto createWindowResult = Window::Create<PromptWindow>(
            instance,
            name,
            name + std::wstring(L"Class"),
            log,
            automation,
            textBoxService,
            promptData.FocusedTextBox,
            promptData.ReleasedVirtualKeyCode,
            promptData.EventWindow
        );
        if (!createWindowResult.has_value())
        {
            log->Fatal(std::string("Failed to create prompt window, error: ")
                .append(GetEnumString(createWindowResult.error())));
            return false;
        }
        promptWindow = std::move(createWindowResult.value());
        promptWindow->GetState().SetTopmost(true);

        focusedTextBoxTracker = std::make_unique<FocusedTextBoxTracker>(
            log,
            automation,
            textBoxService,
            promptData.FocusedTextBox
        );
        auto hr = focusedTextBoxTracker->Start();
        if (FAILED(hr))
        {
            FastLog::HRESULT::Fatal(log, "Failed to start focused textbox tracker", hr);
            return false;
        }

        keyboardTracker = std::make_unique<KeyboardTracker>(log, promptData.ReleasedVirtualKeyCode);
        auto errorCode = keyboardTracker->Start(instance);
        if (errorCode != ERROR_SUCCESS)
        {
            FastLog::Win32::Fatal(log, "Failed to statrt keyboard tracker", errorCode);
            return false;
        }

        windowStateTracker = std::make_unique<WindowStateTracker>(log, promptData.EventWindow);
        errorCode = windowStateTracker->Start();
        if (errorCode != ERROR_SUCCESS)
        {
            FastLog::Win32::Fatal(log, "Failed to start window state tracker", errorCode);
            return false;
        }

        auto createTrayIconResult = TrayIcon::Create(instance, log, std::wstring(applicationName));
        if (!createTrayIconResult.has_value())
        {
            FastLog::Win32::Fatal(log, "Failed to create tray icon", createTrayIconResult.error());
            return false;
        }
        trayIcon = std::move(createTrayIconResult.value());
        if (!trayIcon->Show())
        {
            log->Fatal("Failed to show tray icon");
            return false;
        }

        return true;
    }

    static std::wstring GetApplicationPath(HINSTANCE instance)
    {
        wchar_t path[MAX_PATH] = { 0 };
        GetModuleFileName(instance, path, MAX_PATH);
        return path;
    }

    static std::wstring GetFolderPath(const std::wstring& fullPath)
    {
        std::wstring path = fullPath;
        PathRemoveFileSpec(path.data());

        auto position = path.find(L'\0');
        if (position != std::wstring::npos)
            path.resize(position);

        return path;
    }

    static std::wstring GetFileNameWithoutExtension(const std::wstring& path)
    {
        std::wstring name = path;
        PathStripPath(name.data());
        PathRemoveExtension(name.data());

        auto position = name.find(L'\0');
        if (position != std::wstring::npos)
            name.resize(position);

        return name;
    }

private:
    static constexpr std::wstring_view applicationName = L"InputMethodPrompter";

    PromptData promptData;
    std::shared_ptr<Log> log;
    std::shared_ptr<Automation> automation;
    std::unique_ptr<PromptWindow> promptWindow;
    std::shared_ptr<TextBoxService> textBoxService;
    std::unique_ptr<FocusedTextBoxTracker> focusedTextBoxTracker;
    std::unique_ptr<KeyboardTracker> keyboardTracker;
    std::unique_ptr<WindowStateTracker> windowStateTracker;
    std::unique_ptr<TrayIcon> trayIcon;
};