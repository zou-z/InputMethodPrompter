module;
#include <Windows.h>
export module WindowComponent;

import std;

export class WindowBounds
{
public:
    WindowBounds(HWND handle) : handle(handle) {}

    WindowBounds& GetPosition(int& x, int& y)
    {
        RECT rect;
        GetWindowRect(handle, &rect);
        x = rect.left;
        y = rect.top;
        return *this;
    }

    WindowBounds& GetSize(int& width, int& height)
    {
        RECT rect;
        GetWindowRect(handle, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        return *this;
    }

    WindowBounds& GetBounds(int& x, int& y, int& width, int& height)
    {
        RECT rect;
        GetWindowRect(handle, &rect);
        x = rect.left;
        y = rect.top;
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        return *this;
    }

    WindowBounds& SetPosition(int x, int y)
    {
        SetWindowPos(handle, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return *this;
    }

    WindowBounds& SetSize(int width, int height)
    {
        SetWindowPos(handle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return *this;
    }

    WindowBounds& SetBounds(int x, int y, int width, int height)
    {
        SetWindowPos(handle, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        return *this;
    }

private:
    HWND handle;
};

export class WindowState
{
public:
    WindowState(HWND handle) : handle(handle) {}

    HWND GetHandle() const
    {
        return handle;
    }

    bool IsShowing() const
    {
        return IsWindowVisible(handle) && !IsIconic(handle);
    }

    WindowState& Show()
    {
        if (!IsShowing())
        {
            ShowWindow(handle, SW_SHOWDEFAULT);
            UpdateWindow(handle);
        }
        return *this;
    }

    WindowState& Hide()
    {
        ShowWindow(handle, SW_HIDE);
        return *this;
    }

    WindowState& Minimize()
    {
        ShowWindow(handle, SW_MINIMIZE);
        return *this;
    }

    WindowState& Maximize()
    {
        ShowWindow(handle, SW_MAXIMIZE);
        return *this;
    }

    WindowState& SetTopmost(bool topmost)
    {
        SetWindowPos(
            handle,
            topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );
        return *this;
    }

private:
    HWND handle;
};

export class Window
{
public:
    enum class CreateWindowError
    {
        NullWndProc,
        FailedToRegisterClass,
        FailedToCreateWindow,
    };

    template<std::derived_from<Window> TWindow, typename... Args>
    static std::expected<std::unique_ptr<TWindow>, CreateWindowError> Create(
        HINSTANCE instance,
        const std::wstring& title,
        const std::wstring& className,
        Args&&... args)
    {
        struct WindowAccess :public TWindow
        {
            WindowAccess(Args&&... args) :TWindow(std::forward<Args>(args)...)
            {
            }
        };

        auto windowInstance = std::unique_ptr<TWindow>(
            static_cast<TWindow*>(new WindowAccess(std::forward<Args>(args)...))
        );
        auto window = static_cast<Window*>(windowInstance.get());

        auto windowClass = window->OnCreateWindowClassType(instance, className);
        if (windowClass.lpfnWndProc == nullptr)
            return std::unexpected(CreateWindowError::NullWndProc);
        if ((window->classAtom = RegisterClassExW(&windowClass)) == 0)
            return std::unexpected(CreateWindowError::FailedToRegisterClass);

        window->handle = window->OnCreateWindow(instance, title, className);
        if (window->handle == nullptr)
            return std::unexpected(CreateWindowError::FailedToCreateWindow);

        window->OnInitialized();

        return windowInstance;
    }

    ~Window()
    {
        if (handle != nullptr)
        {
            DestroyWindow(handle);
            UnregisterClassW((LPCWSTR)classAtom, nullptr);
            handle = nullptr;
        }
    }

    WindowBounds GetBounds() const
    {
        return WindowBounds(handle);
    }

    WindowState GetState() const
    {
        return WindowState(handle);
    }

protected:
    Window() :handle(nullptr), classAtom() {}

    virtual WNDCLASSEXW OnCreateWindowClassType(HINSTANCE instance, const std::wstring& className)
    {
        return WNDCLASSEXW
        {
            .cbSize = sizeof(WNDCLASSEXW),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowProcedure,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = instance,
            .hIcon = nullptr,
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = className.c_str(),
            .hIconSm = nullptr,
        };
    }

    virtual HWND OnCreateWindow(HINSTANCE instance, const std::wstring& title, const std::wstring& className)
    {
        return CreateWindowW(
            className.c_str(),
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            0,
            0,
            0,
            0,
            nullptr,
            nullptr,
            instance,
            this
        );
    }

    virtual void OnInitialized()
    {
    }

    virtual LRESULT OnWindowProcedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_DPICHANGED:
        {
            auto oldDpi = dpi;
            dpi = HIWORD(wParam);
            OnDpiChanged(oldDpi, dpi);

            auto suggestedRect = (RECT*)lParam;
            GetBounds().SetBounds(
                suggestedRect->left,
                suggestedRect->top,
                suggestedRect->right - suggestedRect->left,
                suggestedRect->bottom - suggestedRect->top
            );

            break;
        }
        case WM_SIZE:
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);
            OnSizeChanged(width, height);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(handle, message, wParam, lParam);
        }

        return 0;
    }

    virtual void OnDpiChanged(UINT oldDpi, UINT newDpi)
    {
    }

    virtual void OnSizeChanged(UINT width, UINT height)
    {
    }

private:
    static LRESULT CALLBACK WindowProcedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window* self = nullptr;

        if (message == WM_NCCREATE)
        {
            auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<Window*>(createStruct->lpCreateParams);
            SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->dpi = GetDpiForWindow(handle);
        }
        else
        {
            self = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
        }

        return self == nullptr
            ? DefWindowProc(handle, message, wParam, lParam)
            : self->OnWindowProcedure(handle, message, wParam, lParam);
    }

private:
    HWND handle;
    ATOM classAtom;
    UINT dpi = 0;
};