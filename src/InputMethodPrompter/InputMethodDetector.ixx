module;
#include <Windows.h>
#include <imm.h>
export module InputMethodDetector;

export class InputMethodDetector
{
public:
    enum class InputMethodState
    {
        EnglishKeyboard,
        EnglishInputMode,
        NativeInputMode,
    };

    InputMethodDetector() = delete;

    static DWORD GetInputMethodState(HWND windowHandle, InputMethodState& value)
    {
        if (!IsWindow(windowHandle))
            return ERROR_INVALID_WINDOW_HANDLE;

        auto isEnglishKeyboard = false;
        auto result = IsEnglishKeyboard(windowHandle, isEnglishKeyboard);
        if (result != S_OK)
            return result;

        if (isEnglishKeyboard)
        {
            value = InputMethodState::EnglishKeyboard;
            return S_OK;
        }

        auto imeWindowHandle = ImmGetDefaultIMEWnd(windowHandle);
        if (imeWindowHandle == nullptr)
        {
            value = InputMethodState::EnglishKeyboard;
            return S_OK;
        }

        auto openStatus = SendMessage(imeWindowHandle, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
        auto conversionMode = SendMessage(imeWindowHandle, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0);

        value = (openStatus != 0 && (conversionMode & IME_CMODE_NATIVE) != 0)
            ? InputMethodState::NativeInputMode
            : InputMethodState::EnglishInputMode;

        return S_OK;
    }

private:
    static DWORD IsEnglishKeyboard(HWND windowHandle, bool& value)
    {
        auto id = GetWindowThreadProcessId(windowHandle, nullptr);
        if (id == 0)
            return GetLastError();

        auto hkl = GetKeyboardLayout(id);
        auto langId = LOWORD(hkl);
        value = PRIMARYLANGID(langId) == LANG_ENGLISH;

        return S_OK;
    }

private:
    static constexpr auto IMC_GETOPENSTATUS = 0x0005;
    static constexpr auto IMC_GETCONVERSIONMODE = 0x0001;
};