export module Application;

import std;
import PromptWindow;
import TrayIcon;
import Prompter;
import Strings;
import Debug;
import <Windows.h>;

export class Application
{
public:
    Application(HINSTANCE instance) :instance(instance)
    {
        DebugOnly([]
        {
            ConsoleLogger::Open();
            ConsoleLogger::Info(L"Application started");
        });
        Initialize();
    }

    int Run()
    {
        auto singletonMutex = CreateMutex(nullptr, TRUE, L"InputMethodPrompter_SingletonMutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(singletonMutex);
            MessageBox(nullptr, Strings::ApplicationAlreadyRunning.data(), Strings::ApplicationName.data(), MB_OK | MB_ICONEXCLAMATION);
            return 0;
        }

        DebugOnly([] { ConsoleLogger::Info(L"Application running..."); });

        MSG message;
        while (GetMessage(&message, nullptr, 0, 0))
        {
            if (!TranslateAccelerator(message.hwnd, nullptr, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }

        Uninitialize();

        if (singletonMutex != nullptr)
        {
            ReleaseMutex(singletonMutex);
            CloseHandle(singletonMutex);
        }

        return static_cast<int>(message.wParam);
    }

    void Initialize()
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        mainWindow = std::make_shared<PromptWindow>(
            instance,
            Strings::ApplicationName.data(),
            Strings::ApplicationName.data() + std::wstring(L" Class")
        );
        mainWindow->Initialize(28, 28);

        trayIcon = std::make_unique<TrayIcon>();
        trayIcon->Init(instance);

        prompter = std::make_unique<Prompter>(mainWindow);
        prompter->Initialize(instance);
    }

    void Uninitialize()
    {
    }

private:
    const HINSTANCE instance;
    std::shared_ptr<PromptWindow> mainWindow;
    std::unique_ptr<TrayIcon> trayIcon;
    std::unique_ptr<Prompter> prompter;
};
