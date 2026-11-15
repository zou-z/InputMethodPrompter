module;
#include <chrono>
#include <fstream>
#include <Windows.h>
export module LogComponent;

import std;

export enum LogLevel { Info, Warn, Error, Fatal };

export struct ILog
{
    virtual ~ILog() = default;
    virtual void Info(const std::string& message) = 0;
    virtual void Warn(const std::string& message) = 0;
    virtual void Error(const std::string& message) = 0;
    virtual void Fatal(const std::string& message) = 0;
};

export class Log :public ILog
{
public:
    Log(const std::wstring& path, const std::wstring& name, LogLevel logLevel) :
        name(name),
        path(path + L"\\" + name + L".log"),
        logLevel(logLevel)
    {
    }

    void Info(const std::string& message) override
    {
        LogMessage(LogLevel::Info, message);
    }

    void Warn(const std::string& message) override
    {
        LogMessage(LogLevel::Warn, message);
    }

    void Error(const std::string& message) override
    {
        LogMessage(LogLevel::Error, message);
    }

    void Fatal(const std::string& message) override
    {
        LogMessage(LogLevel::Fatal, message);
    }

private:
    void LogMessage(LogLevel level, const std::string& message)
    {
        if (isDebug)
        {
            if (level >= LogLevel::Error)
            {
                auto title = std::format(L"{0} - {1}", StringToWstring(GetLevelText(level)), name);
                MessageBox(nullptr, StringToWstring(message).c_str(), title.c_str(), MB_OK | MB_ICONERROR);
            }
        }

        if (level < logLevel)
            return;

        auto text = std::format("[{0}][{1}] {2}", GetCurrentLocalTime(), GetLevelText(level), message);
        WriteLogToFile(text);
    }

    void WriteLogToFile(const std::string& text)
    {
        if (!file.is_open())
        {
            file.clear();
            file.open(path, std::ios::app);
        }

        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.put('\n');
        file.flush();
    }

    static std::string GetLevelText(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Info: return "Info";
        case LogLevel::Warn: return "Warn";
        case LogLevel::Error: return "Error";
        case LogLevel::Fatal: return "Fatal";
        default: return "";
        }
    }

    static std::string GetCurrentLocalTime()
    {
        using namespace std::chrono;
        const auto utcTime = floor<milliseconds>(system_clock::now());
        const auto localTime = zoned_time(current_zone(), utcTime);
        return std::format("{:%Y-%m-%d %H:%M:%S}", localTime);
    }

    static std::wstring StringToWstring(const std::string& str, UINT codePage = CP_UTF8)
    {
        if (str.empty()) return std::wstring();

        int len = MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), nullptr, 0);
        if (len <= 0) return std::wstring();

        std::wstring result(len, L'\0');
        MultiByteToWideChar(codePage, 0, str.c_str(), (int)str.size(), &result[0], len);
        return result;
    }

private:
#ifdef _DEBUG
    static constexpr bool isDebug = true;
#else
    static constexpr bool isDebug = false;
#endif
    std::wstring name;
    std::wstring path;
    LogLevel logLevel;
    std::ofstream file;
};

export std::string HResultToString(HRESULT result)
{
    return std::format("0x{:08X}", static_cast<std::uint32_t>(result));
}