export module Window;

import std;
import WindowExtensions;
import <Windows.h>;

export class Window
{
public:
    Window(
        HINSTANCE instance,
        const std::wstring title,
        const std::wstring className = L"",
        bool isTransparent = false) :
        instance(instance),
        title(title),
        isTransparent(isTransparent),
        className(className.empty() ? title + L"Class" : className)
    {
    }

    ~Window()
    {
        Destroy();
    }

    HWND GetHandle() const
    {
        return handle;
    }

    void Initialize()
    {
        if (handle == nullptr)
        {
            RegisterWindowClass(instance, this->className);
            handle = OnCreateWindow(instance, title, this->className);
        }
    }

    WindowBoundary GetBoundary() const
    {
        return WindowBoundary(handle);
    }

    WindowStatus GetStatus() const
    {
        return WindowStatus(handle);
    }

    void Destroy()
    {
        if (handle != nullptr)
        {
            DestroyWindow(handle);
            handle = nullptr;
            UnregisterClassW(className.c_str(), nullptr);
        }
    }

protected:
    virtual HWND OnCreateWindow(HINSTANCE instance, const std::wstring title, const std::wstring className)
    {
        return CreateWindowW(
            className.c_str(),
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            0,
            CW_USEDEFAULT,
            0,
            nullptr,
            nullptr,
            instance,
            this
        );
    }

    virtual LRESULT OnWindowProcedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_DPICHANGED:
        {
            UINT newDpi = HIWORD(wParam);
            auto scale = (float)newDpi / (float)dpi;

            RECT rect;
            GetWindowRect(handle, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            int newWidth = (int)(width * scale);
            int newHeight = (int)(height * scale);

            auto suggestedRect = (RECT*)lParam;
            SetWindowPos(
                handle,
                nullptr,
                suggestedRect->left,
                suggestedRect->top,
                newWidth,
                newHeight,
                SWP_NOZORDER | SWP_NOACTIVATE
            );

            dpi = newDpi;
        }
        break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(handle, message, wParam, lParam);
        }

        return 0;
    }

    UINT GetDpi() const
    {
        return dpi;
    }

private:
    ATOM RegisterWindowClass(HINSTANCE instance, const std::wstring classTitle)
    {
        WNDCLASSEXW windowClass
        {
            .cbSize = sizeof(WNDCLASSEXW),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WindowProcedure,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = instance,
            .hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON)),
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = isTransparent ? nullptr : (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = classTitle.c_str(),
            .hIconSm = (HICON)LoadImage(instance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR),
        };

        return RegisterClassExW(&windowClass);
    }

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
    static constexpr int IDI_APPICON = 1;
    const std::wstring title;
    const std::wstring className;
    const HINSTANCE instance;
    const bool isTransparent;
    HWND handle = nullptr;
    UINT dpi = 96;
};
