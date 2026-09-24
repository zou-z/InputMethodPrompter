module;
#include <Windows.h>
export module WindowTests;

import WindowComponent;
import Strings;


class TestWindow1 : public Window
{
public:

protected:
    TestWindow1() {}

    WNDCLASSEXW OnCreateWindowClassType(HINSTANCE instance, const std::wstring& className) override
    {
        auto windowClass = Window::OnCreateWindowClassType(instance, className);
        windowClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON));
        windowClass.hIconSm = (HICON)LoadImage(instance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        //windowClass.hbrBackground = nullptr;
        return windowClass;
    }

private:
    static constexpr int IDI_APPICON = 1;
};


export class WindowTests
{
public:
    WindowTests()
    {
    }

    int RunTests(HINSTANCE instance)
    {
        auto result = Window::Create<TestWindow1>(
            instance,
            Strings::ApplicationName.data(),
            Strings::ApplicationName.data() + std::wstring(L" Class")
        );
        if (!result.has_value())
            return 1;

        std::unique_ptr<TestWindow1> window = std::move(result.value());
        window->GetBounds().SetBounds(10, 10, 400, 300);
        window->GetState().Show();
        window->GetState().SetTopmost(true);

        MSG message;
        while (GetMessage(&message, nullptr, 0, 0))
        {
            if (!TranslateAccelerator(message.hwnd, nullptr, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }

        return static_cast<int>(message.wParam);
    }
};