export module PromptWindow;

import std;
import Window;
import InputMethodDetector;
import <Windows.h>;
import <dwmapi.h>;

export class PromptWindow : public Window
{
public:
    PromptWindow(
        HINSTANCE instance,
        const std::wstring title,
        const std::wstring className = L"") :
        Window(instance, title, className, true)
    {
    }

    void SetContent(InputMethodDetector::InputMethodState state)
    {
        auto newContent = GetContentFromState(state);
        if (content != newContent)
        {
            content = newContent;
            InvalidateRect(GetHandle(), nullptr, TRUE);
        }
    }

protected:
    HWND OnCreateWindow(HINSTANCE instance, const std::wstring title, const std::wstring className) override
    {
        // WS_POPUP 没有标题栏和边框
        // WS_EX_LAYERED 启用分层窗口（支持透明度）
        // WS_EX_TRANSPARENT 鼠标穿透窗口
        // WS_EX_TOPMOST 窗口置顶
        auto dwStyle = WS_POPUP;
        auto dwExStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST;

        auto handle = CreateWindowExW(
            dwExStyle,
            className.c_str(),
            title.c_str(),
            dwStyle,
            CW_USEDEFAULT,
            0,
            CW_USEDEFAULT,
            0,
            nullptr,
            nullptr,
            instance,
            this
        );

        SetLayeredWindowAttributes(handle, RGB(0, 0, 0), 0, LWA_COLORKEY);

        // 去掉任务栏图标
        SetWindowLong(handle, GWL_EXSTYLE, GetWindowLong(handle, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);

        // 设置圆角（0=默认, 1=无圆角, 2=小圆角, 3=大圆角）
        int preference = 3;
        DwmSetWindowAttribute(handle, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

        return handle;
    }

    LRESULT OnWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override
    {
        switch (message)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            auto scale = GetDpi() / 96.0f;

            DrawBackground(hdc, rect);
            DrawTextContent(hdc, scale, rect);

            EndPaint(hWnd, &ps);
        }
        break;
        }

        return Window::OnWindowProcedure(hWnd, message, wParam, lParam);
    }

private:
    void DrawBackground(HDC hdc, const RECT& rect)
    {
        auto brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }

    void DrawTextContent(HDC hdc, float scale, const RECT& rect) const
    {
        // 创建自定义字体
        LOGFONT logFont = { 0 };
        logFont.lfHeight = static_cast<LONG>(fontSize * scale);
        logFont.lfWeight = FW_NORMAL;
        lstrcpy(logFont.lfFaceName, L"Segoe Fluent Icons");

        auto font = CreateFontIndirect(&logFont);
        auto oldFont = (HFONT)SelectObject(hdc, font);

        // 背景透明
        SetBkMode(hdc, TRANSPARENT);

        // 文本颜色
        SetTextColor(hdc, RGB(0, 128, 255));

        SetTextCharacterExtra(hdc, 0);
        SetGraphicsMode(hdc, GM_ADVANCED);
        SetTextAlign(hdc, TA_LEFT | TA_TOP);

        DrawText(hdc, content.c_str(), -1, (RECT*)&rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    static std::wstring GetContentFromState(InputMethodDetector::InputMethodState state)
    {
        switch (state)
        {
        case InputMethodDetector::InputMethodState::EnglishKeyboard: return L"\uE765";
        case InputMethodDetector::InputMethodState::EnglishInputMode: return L"\uE983";
        case InputMethodDetector::InputMethodState::NativeInputMode: return L"\uE982";
        default: return L"";
        }
    }

private:
    static constexpr int fontSize = 16;
    std::wstring content;
};
