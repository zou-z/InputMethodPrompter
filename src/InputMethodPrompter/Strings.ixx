export module Strings;

import std;

export namespace Strings
{
    constexpr std::wstring_view ApplicationName = L"输入法提示器";
    constexpr std::wstring_view ApplicationVersion = L"1.0.1";
    constexpr std::wstring_view ApplicationLink = L"https://github.com/zou-z/InputMethodPrompter";
    constexpr std::wstring_view ApplicationAlreadyRunning = L"程序已经在运行中。";
    constexpr std::wstring_view ApplicationAutoStart = L"开机自启动";
    constexpr std::wstring_view Exit = L"退出";
    constexpr std::wstring_view About = L"关于";
}