module;
#include <Windows.h>
export module WindowTests;

import WindowV2;
import Strings;


class GaussianBlurWindow : public WindowV2
{
public:

protected:
    GaussianBlurWindow() {}

    virtual WNDCLASSEXW OnCreateWindowClassType(HINSTANCE instance) override
    {
        auto windowClass = WindowV2::OnCreateWindowClassType(instance);
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
        auto result = WindowV2::Create<GaussianBlurWindow>(
            instance,
            Strings::ApplicationName.data(),
            Strings::ApplicationName.data() + std::wstring(L" Class")
        );
        if (!result.has_value())
            return 1;

        std::unique_ptr<GaussianBlurWindow> window = std::move(result.value());
        window->GetBounds().SetBounds(10, 10, 400, 300);
        window->GetState().Show();

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