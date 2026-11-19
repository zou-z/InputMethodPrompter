module;
#include <Windows.h>
#include <imm.h>
export module InputMethodService;

export enum class InputMethodState
{
    Unknown,
    EnglishKeyboard,
    EnglishInputMode,
    ChineseInputMode,
};

export struct KeyboardState
{
    bool IsNumLocked;
    bool IsCapsLocked;
    bool IsScrollLocked;
    bool operator==(const KeyboardState&) const = default;
};

export struct InputState
{
    InputMethodState MethodState;
    KeyboardState KeyboardState;
    bool operator==(const InputState&) const = default;
};

export class InputMethodDetector
{
public:
    InputMethodDetector() = delete;

    static DWORD GetInputState(HWND windowHandle, InputState& state)
    {
        auto errorCode = GetInputMethodState(windowHandle, state.MethodState);
        if (errorCode != ERROR_SUCCESS)
            return errorCode;

        state.KeyboardState.IsNumLocked = IsLockEnabled(VK_NUMLOCK);
        state.KeyboardState.IsCapsLocked = IsLockEnabled(VK_CAPITAL);
        state.KeyboardState.IsScrollLocked = IsLockEnabled(VK_SCROLL);

        return ERROR_SUCCESS;
    }

private:
    static DWORD GetInputMethodState(HWND windowHandle, InputMethodState& state)
    {
        if (!IsWindow(windowHandle))
            return ERROR_INVALID_WINDOW_HANDLE;

        auto isEnglishKeyboard = false;
        auto result = IsEnglishKeyboard(windowHandle, isEnglishKeyboard);
        if (result != ERROR_SUCCESS)
            return result;

        if (isEnglishKeyboard)
        {
            state = InputMethodState::EnglishKeyboard;
            return ERROR_SUCCESS;
        }

        auto imeWindowHandle = ImmGetDefaultIMEWnd(windowHandle);
        if (imeWindowHandle == nullptr)
        {
            state = InputMethodState::EnglishKeyboard;
            return ERROR_SUCCESS;
        }

        constexpr auto IMC_GETOPENSTATUS = 0x0005;
        constexpr auto IMC_GETCONVERSIONMODE = 0x0001;
        auto openStatus = SendMessage(imeWindowHandle, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
        auto conversionMode = SendMessage(imeWindowHandle, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0);

        state = (openStatus != 0 && (conversionMode & IME_CMODE_NATIVE) != 0)
            ? InputMethodState::ChineseInputMode
            : InputMethodState::EnglishInputMode;

        return ERROR_SUCCESS;
    }

    static bool IsLockEnabled(int virtualKey)
    {
        return (GetKeyState(virtualKey) & 0x0001) != 0;
    }

    static DWORD IsEnglishKeyboard(HWND windowHandle, bool& value)
    {
        auto id = GetWindowThreadProcessId(windowHandle, nullptr);
        if (id == 0)
            return GetLastError();

        auto hkl = GetKeyboardLayout(id);
        auto langId = LOWORD(hkl);
        value = PRIMARYLANGID(langId) == LANG_ENGLISH;

        return ERROR_SUCCESS;
    }
};