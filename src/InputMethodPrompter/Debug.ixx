export module Debug;

import std;
import <Windows.h>;
import <chrono>;


constexpr bool isDebug =
#ifdef _DEBUG
true;
#else
false;
#endif


export template<typename F>
inline void DebugOnly(F&& f)
{
    if constexpr (isDebug)
    {
        f();
    }
}


export class ConsoleLogger
{
private:
    enum class Level
    {
        Info,
        Warn,
        Error,
    };

public:
    ConsoleLogger() = delete;

    static bool IsOpen()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return isOpened;
    }

    static void Open()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (isOpened)
            return;

        if (AllocConsole() == FALSE)
            return;

        consoleHandle = CreateFileW(
            L"CONOUT$",
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        isOpened = true;
    }

    static void Close()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!isOpened)
            return;

        FreeConsole();
        consoleHandle = nullptr;
        isOpened = false;
    }

    static void Info(const std::wstring& message)
    {
        Log(Level::Info, message);
    }

    static void Warn(const std::wstring& message)
    {
        Log(Level::Warn, message);
    }

    static void Error(const std::wstring& message)
    {
        Log(Level::Error, message);
    }

private:
    static void SetColor(Level level)
    {
        WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        switch (level)
        {
        case Level::Info:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
        case Level::Warn:
            color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case Level::Error:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        }

        SetConsoleTextAttribute(consoleHandle, color);
    }

    static void ResetColor()
    {
        SetConsoleTextAttribute(
            consoleHandle,
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
        );
    }

    static void Log(Level level, const std::wstring& message)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!isOpened)
            return;

        PrintTimestamp();
        PrintLogLevel(level);
        PrintContent(message + L"\r\n");
    }

    static void PrintTimestamp()
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::time_t tt = system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &tt);

        wchar_t buffer[64];
        swprintf_s(
            buffer,
            L"[%02d:%02d:%02d.%03lld]",
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            static_cast<long long>(ms.count())
        );

        PrintContent(buffer);
    }

    static void PrintLogLevel(Level level)
    {
        PrintContent(L"[");
        SetColor(level);

        switch (level)
        {
        case Level::Info: PrintContent(L"INFO "); break;
        case Level::Warn: PrintContent(L"WARN "); break;
        case Level::Error: PrintContent(L"ERROR"); break;
        }

        ResetColor();
        PrintContent(L"]");
    }

    static void PrintContent(const std::wstring& content)
    {
        DWORD written = 0;
        WriteConsoleW(consoleHandle, content.c_str(), (DWORD)content.size(), &written, nullptr);
    }

private:
    inline static bool isOpened = false;
    inline static HANDLE consoleHandle;
    inline static std::mutex mutex;
};


export inline void OutputErrorMessage(const std::wstring& message)
{
    if constexpr (isDebug)
    {
        ConsoleLogger::Error(message);
    }
    else
    {
        OutputDebugString((message + L"\r\n").c_str());
    }
}
