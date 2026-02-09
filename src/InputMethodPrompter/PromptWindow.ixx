export module PromptWindow;

import std;
import Debug;
import Window;
import PromptContentRenderer;
import InputMethodDetector;
import <Windows.h>;

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

    void SetContent(InputMethodDetector::InputMethodState state, bool isCapsLockToggled)
    {
        if (renderer.SetContent(state, isCapsLockToggled))
        {
            auto hr = renderer.Render();
            if (FAILED(hr))
                OutputErrorMessage(L"Renderer render failed after setting content " + std::to_wstring(hr));
        }
    }

protected:
    HWND OnCreateWindow(
        HINSTANCE instance,
        const std::wstring title,
        const std::wstring className,
        UINT width,
        UINT height) override
    {
        auto dwStyle = WS_POPUP;
        auto dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

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

        auto scale = GetDpi() / 96.0f;
        width = (UINT)(width * scale);
        height = (UINT)(height * scale);
        SetWindowPos(handle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);

        auto hr = renderer.Initialize(handle, width, height, scale);
        if (FAILED(hr))
            OutputErrorMessage(L"Renderer initialize failed " + std::to_wstring(hr));

        return handle;
    }

    LRESULT OnWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override
    {
        switch (message)
        {
        case WM_DPICHANGED:
        {
            auto dpi = HIWORD(wParam);
            auto suggestedRect = (RECT*)lParam;
            auto width = suggestedRect->right - suggestedRect->left;
            auto height = suggestedRect->bottom - suggestedRect->top;
            auto scale = dpi / 96.0f;

            auto hr = renderer.Resize(width, height, scale);
            if (FAILED(hr))
            {
                OutputErrorMessage(L"Renderer resize failed " + std::to_wstring(hr));
            }
            else
            {
                hr = renderer.Render();
                if (FAILED(hr))
                    OutputErrorMessage(L"Renderer render failed " + std::to_wstring(hr));
            }
        }
        break;
        }

        return Window::OnWindowProcedure(hWnd, message, wParam, lParam);
    }

private:
    PromptContentRenderer renderer;
};
