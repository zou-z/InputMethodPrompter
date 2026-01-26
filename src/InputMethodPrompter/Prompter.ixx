module;
#include <winrt/base.h>
#include <UIAutomation.h>
#include <tlhelp32.h>
export module Prompter;

import std;
import PromptWindow;
import EditorRecognizer;
import InputMethodDetector;
import DebouncedAction;
import KeyboardHook;
import WinEventHook;

export class Prompter
{
private:
    class FocusChangedHandler : public IUIAutomationFocusChangedEventHandler
    {
    public:
        FocusChangedHandler(std::function<void(IUIAutomationElement*)> onFocusChanged) :
            onFocusChanged(onFocusChanged)
        {
        }

        ULONG STDMETHODCALLTYPE AddRef(void) override
        {
            return InterlockedIncrement(&referenceCount);
        }

        ULONG STDMETHODCALLTYPE Release(void) override
        {
            auto count = InterlockedDecrement(&referenceCount);
            if (count == 0)
                delete this;

            return count;
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
        {
            if (riid == IID_IUnknown || riid == IID_IUIAutomationFocusChangedEventHandler)
            {
                *ppvObject = static_cast<IUIAutomationFocusChangedEventHandler*>(this);
                AddRef();
                return S_OK;
            }

            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(IUIAutomationElement* sender) override
        {
            onFocusChanged(sender);
            return S_OK;
        }

    private:
        ULONG referenceCount = 1;
        std::function<void(IUIAutomationElement*)> onFocusChanged;
    };

    class InputMethodUtility
    {
    public:
        InputMethodUtility() = delete;

        static bool IsTextInputHostProcess(int processId)
        {
            if (textInputHostProcessId == -1 && GetProcessNameByProcessId(processId) == L"TextInputHost.exe")
                textInputHostProcessId = processId;

            return processId != -1 && processId == textInputHostProcessId;
        }

    private:
        static std::wstring GetProcessNameByProcessId(DWORD processId)
        {
            std::wstring processName = L"";

            auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return processName;

            PROCESSENTRY32W processEntry
            {
                .dwSize = sizeof(PROCESSENTRY32W)
            };

            if (Process32FirstW(snapshot, &processEntry))
            {
                do
                {
                    if (processEntry.th32ProcessID == processId)
                    {
                        processName = processEntry.szExeFile;
                        break;
                    }
                } while (Process32NextW(snapshot, &processEntry));
            }

            CloseHandle(snapshot);
            return processName;
        }

    private:
        inline static int textInputHostProcessId = -1;
    };

public:
    Prompter(std::shared_ptr<PromptWindow> window) :
        window(window),
        handleKeyEventAction([this]() { HandleKeyEvent(); }),
        handleWinEventAction([this]() { HandleWinEvent(); })
    {
    }

    ~Prompter()
    {
        KeyboardHook::Uninstall();
        WinEventHook::Uninstall();
        CoUninitialize();
    }

    HRESULT Initialize(HINSTANCE instance)
    {
        auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return hr;

        hr = CoCreateInstance(
            CLSID_CUIAutomation,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(automation.put())
        );
        if (FAILED(hr))
            return hr;

        if (focusChangedHandler == nullptr)
            focusChangedHandler.attach(new FocusChangedHandler([this](IUIAutomationElement* element) { OnFocusChanged(element); }));

        hr = automation->AddFocusChangedEventHandler(nullptr, focusChangedHandler.get());
        if (FAILED(hr))
            return hr;

        auto result = KeyboardHook::Install(instance, [this](WPARAM wParam, LPARAM lParam) { OnKeyEvent(wParam, lParam); });
        if (FAILED(result))
            return HRESULT_FROM_WIN32(result);

        result = WinEventHook::Install([this](HWND handle) { OnWinEvent(handle); });
        if (FAILED(result))
            return HRESULT_FROM_WIN32(result);

        return S_OK;
    }

private:
    void OnFocusChanged(IUIAutomationElement* element)
    {
        HWND handle = nullptr;
        RECT position;
        if (editorRecognizerService.GetEditorPosition(automation.get(), element, position))
        {
            handle = GetHandleFromElement(automation.get(), element);
            auto inputMethodState = InputMethodDetector::InputMethodState::EnglishKeyboard;
            if (InputMethodDetector::GetInputMethodState(handle, inputMethodState) == S_OK)
            {
                window->GetBoundary().SetPosition(position.left, position.bottom);
                window->SetContent(inputMethodState);
                window->GetStatus().Show();
            }

            std::lock_guard<std::mutex> lock(mutex);
            lastFocusedElement.copy_from(element);
            lastFocusedHandle = handle;
        }
        else
        {
            int processId = -1;
            element->get_CurrentProcessId(&processId);

            if (!InputMethodUtility::IsTextInputHostProcess(processId))
                window->GetStatus().Hide();

            std::lock_guard<std::mutex> lock(mutex);
            lastFocusedElement = nullptr;
            lastFocusedHandle = nullptr;
        }
    }

    void OnKeyEvent(WPARAM wParam, LPARAM lParam)
    {
        HWND lastHandle = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex);
            lastHandle = lastFocusedHandle;
        }

        if (!window->GetStatus().IsShowing() || lastHandle == nullptr)
            return;

        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            auto keyEvent = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

            auto isShiftReleased = keyEvent->vkCode == VK_SHIFT || keyEvent->vkCode == VK_LSHIFT || keyEvent->vkCode == VK_RSHIFT;
            auto isSpaceReleased = keyEvent->vkCode == VK_SPACE;

            auto isSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
            auto isCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
                || (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0
                || (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
            auto isWinPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;

            // Shift松开
            // Ctrl按下+Space松开，Ctrl按下+Space按下
            // Win按下+Space松开，Win按下+Space按下
            if (isShiftReleased
                || isCtrlPressed && isSpaceReleased || isCtrlPressed && isSpacePressed
                || isWinPressed && isSpaceReleased || isWinPressed && isSpacePressed)
            {
                handleKeyEventAction.Request();
            }
        }
    }

    void OnWinEvent(HWND handle)
    {
        HWND lastHandle = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex);
            lastHandle = lastFocusedHandle;
        }

        if (!window->GetStatus().IsShowing() || lastHandle == nullptr)
            return;

        if (lastHandle == handle || GetAncestor(lastHandle, GA_ROOT) == handle)
            handleWinEventAction.Request();
    }

    void HandleKeyEvent()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        HWND lastHandle = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex);
            lastHandle = lastFocusedHandle;
        }

        if (lastHandle == nullptr)
            return;

        auto inputMethodState = InputMethodDetector::InputMethodState::EnglishKeyboard;
        if (InputMethodDetector::GetInputMethodState(lastHandle, inputMethodState) == S_OK)
        {
            window->SetContent(inputMethodState);
        }
    }

    void HandleWinEvent()
    {
        IUIAutomationElement* lastElement = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex);
            lastElement = lastFocusedElement.get();
        }

        if (lastElement == nullptr)
            return;

        RECT position;
        if (editorRecognizerService.GetEditorPosition(automation.get(), lastElement, position))
            window->GetBoundary().SetPosition(position.left, position.bottom);
    }

    static HWND GetHandleFromElement(IUIAutomation* automation, IUIAutomationElement* element)
    {
        HWND handle = nullptr;
        IUIAutomationElement* currentElement = element;
        winrt::com_ptr<IUIAutomationTreeWalker> walker;

        while (currentElement != nullptr)
        {
            auto hr = currentElement->get_CurrentNativeWindowHandle(reinterpret_cast<UIA_HWND*>(&handle));
            if (SUCCEEDED(hr) && handle != nullptr)
                return handle;

            if (walker == nullptr)
                automation->get_ControlViewWalker(walker.put());

            IUIAutomationElement* parentElement = nullptr;
            currentElement = SUCCEEDED(walker->GetParentElement(currentElement, &parentElement))
                ? parentElement
                : nullptr;
        }

        return nullptr;
    }

private:
    winrt::com_ptr<IUIAutomation> automation;
    winrt::com_ptr<FocusChangedHandler> focusChangedHandler;
    std::shared_ptr<PromptWindow> window;
    std::mutex mutex;
    EditorRecognizerService editorRecognizerService;

    DebouncedAction handleKeyEventAction;
    DebouncedAction handleWinEventAction;

    HWND lastFocusedHandle = nullptr;
    winrt::com_ptr<IUIAutomationElement> lastFocusedElement;
};
