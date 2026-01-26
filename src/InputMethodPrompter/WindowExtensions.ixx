export module WindowExtensions;

import <Windows.h>;

struct WindowExtensions
{
protected:
    WindowExtensions(HWND handle) :Handle(handle)
    {
    }

    HWND Handle;
};

export struct WindowBoundary : WindowExtensions
{
    WindowBoundary(HWND handle) : WindowExtensions(handle)
    {
    }

    void GetPosition(int& x, int& y) const
    {
        RECT rect;
        GetWindowRect(Handle, &rect);
        x = rect.left;
        y = rect.top;
    }

    void GetSize(int& width, int& height) const
    {
        RECT rect;
        GetWindowRect(Handle, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    void SetPosition(int x, int y) const
    {
        SetWindowPos(Handle, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void SetSize(int width, int height) const
    {
        SetWindowPos(Handle, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    void SetPositionAndSize(int x, int y, int width, int height) const
    {
        SetWindowPos(Handle, nullptr, x, y, width, height, SWP_NOZORDER);
    }
};

export struct WindowStatus : WindowExtensions
{
    WindowStatus(HWND handle) : WindowExtensions(handle)
    {
    }

    bool IsShowing() const
    {
        return IsWindowVisible(Handle) && !IsIconic(Handle);
    }

    void Show() const
    {
        ShowWindow(Handle, SW_SHOWDEFAULT);
        UpdateWindow(Handle);
    }

    void Hide() const
    {
        ShowWindow(Handle, SW_HIDE);
    }

    void Maximize() const
    {
        ShowWindow(Handle, SW_MAXIMIZE);
    }

    void Minimize() const
    {
        ShowWindow(Handle, SW_MINIMIZE);
    }

    void SetTopmost(bool topmost) const
    {
        SetWindowPos(
            Handle,
            topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE
        );
    }
};