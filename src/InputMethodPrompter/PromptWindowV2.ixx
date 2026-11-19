module;
#include <dwrite.h>
#include "GraphicsTypes.h"
#include "Macros.h"
export module PromptWindowV2;

import std;
import PromptData;
import LogComponent;
import WindowComponent;
import RenderComponent;
import ReactiveComponent;
import AutomationComponent;
import TextBoxService;
import EnumComponent;
import InputMethodDetectorV2;
import ConflationComponent;

using namespace Reactive;

class PromptRenderer
{
public:
    PromptRenderer(Window* window, GraphicsResources resources) :
        window(window),
        resources(std::move(resources)),
        state({ .MethodState = InputMethodState::Unknown, .KeyboardState = { false } })
    {
    }

    HRESULT Resize(UINT width, UINT height)
    {
        return GraphicsUtilities::Resize(
            resources.D3D11DeviceContext,
            resources.D2D1DeviceContext,
            resources.SwapChain,
            resources.D2D1Bitmap,
            width,
            height
        );
    }

    HRESULT RenderInputState(const InputState& state)
    {
        RETURN_IF(this->state == state, S_OK);
        this->state = state;
        return Render();
    }

private:
    HRESULT Render()
    {
        HRESULT hr = S_OK;

        float width = 0;
        float height = 0;
        ResizeWindow(width, height);

        GraphicsRender::BeginDraw(resources, D2D1::ColorF(0, 0, 0, 0));
        RETURN_IF_FAILED(RenderBackground(width, height), hr);
        RETURN_IF_FAILED(
            RenderInputMethodStatePrompt(RectF(
                windowBorderThickness,
                windowBorderThickness,
                height - windowBorderThickness * 2,
                height - windowBorderThickness * 2)),
            hr
        );
        RETURN_IF_FAILED(RenderKeyboardStatePrompt(height - windowBorderThickness, height), hr);
        RETURN_IF_FAILED(GraphicsRender::EndDraw(resources), hr);

        return hr;
    }

    HRESULT RenderBackground(float width, float height) const
    {
        return GraphicsRender::DrawBorder(
            resources,
            RectF(0, 0, width, height),
            windowCornerRadius,
            windowBorderThickness,
            D2D1::ColorF(D2D1::ColorF::Gainsboro),
            D2D1::ColorF(D2D1::ColorF::White)
        );
    }

    HRESULT RenderInputMethodStatePrompt(const RectF& position) const
    {
        std::wstring text = L"";
        std::wstring fontFamily = L"Segoe UI";
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

        if (state.MethodState == InputMethodState::EnglishInputMode
            || state.MethodState == InputMethodState::ChineseInputMode)
        {
            text = state.MethodState == InputMethodState::EnglishInputMode ? L"英" : L"中";
        }
        else if (state.MethodState == InputMethodState::EnglishKeyboard)
        {
            text = L"\uE765";
            fontFamily = L"Segoe MDL2 Assets";
            paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        }

        return GraphicsRender::DrawString(
            resources,
            text,
            fontFamily,
            16,
            D2D1::ColorF(21 / 255.0f, 115 / 255.0f, 253 / 255.0f),
            position,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            paragraphAlignment
        );
    }

    HRESULT RenderKeyboardStatePrompt(float leftOffset, float height) const
    {
        constexpr int separatorHeight = 12;
        constexpr int stateHeight = 16;

        HRESULT hr = S_OK;
        auto& keyboardState = state.KeyboardState;
        RETURN_IF(!keyboardState.IsNumLocked && !keyboardState.IsCapsLocked && !keyboardState.IsScrollLocked, hr);

        auto top = (height - separatorHeight) / 2;
        RETURN_IF_FAILED(
            GraphicsRender::FillRectangle(
                resources,
                RectF(leftOffset, top, separatorWidth, separatorHeight),
                D2D1::ColorF(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 1)),
            hr
        );
        top = (height - stateHeight) / 2;
        leftOffset += separatorWidth + keyboardStateHorizontalPadding;

        if (keyboardState.IsNumLocked)
        {
            RETURN_IF_FAILED(RenderKeyboardStateSymbol(RectF(leftOffset, top, keyboardStateWidth, stateHeight), L"N"), hr);
            leftOffset += keyboardStateWidth + keyboardStateHorizontalPadding;
        }

        if (keyboardState.IsCapsLocked)
        {
            RETURN_IF_FAILED(RenderKeyboardStateSymbol(RectF(leftOffset, top, keyboardStateWidth, stateHeight), L"C"), hr);
            leftOffset += keyboardStateWidth + keyboardStateHorizontalPadding;
        }

        if (keyboardState.IsScrollLocked)
        {
            RETURN_IF_FAILED(RenderKeyboardStateSymbol(RectF(leftOffset, top, keyboardStateWidth, stateHeight), L"S"), hr);
        }

        return hr;
    }

    HRESULT RenderKeyboardStateSymbol(const RectF& position, const std::wstring& symbol) const
    {
        HRESULT hr = S_OK;

        RETURN_IF_FAILED(GraphicsRender::DrawBorder(resources, position, 4, D2D1::ColorF(D2D1::ColorF::Black)), hr);
        RETURN_IF_FAILED(
            GraphicsRender::DrawString(
                resources,
                symbol,
                L"Consolas",
                13,
                D2D1::ColorF(D2D1::ColorF::White),
                position,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER),
            hr
        );

        return hr;
    }

    void ResizeWindow(float& width, float& height)
    {
        constexpr int expectedHeight = 24;
        int expectedWidth = expectedHeight;

        int lockedCount = 0;
        if (state.KeyboardState.IsNumLocked) ++lockedCount;
        if (state.KeyboardState.IsCapsLocked) ++lockedCount;
        if (state.KeyboardState.IsScrollLocked) ++lockedCount;
        if (lockedCount > 0)
        {
            expectedWidth += separatorWidth;
            expectedWidth += keyboardStateHorizontalPadding * (lockedCount + 1);
            expectedWidth += keyboardStateWidth * lockedCount;
        }

        int windowWidth = 0;
        int windowHeight = 0;
        window->GetBounds().GetSize(windowWidth, windowHeight);
        if (windowWidth != expectedWidth || windowHeight != expectedHeight)
            window->GetBounds().SetSize(expectedWidth, expectedHeight);

        width = static_cast<float>(expectedWidth);
        height = static_cast<float>(expectedHeight);
    }

private:
    const Window* window;
    GraphicsResources resources;
    InputState state;
    static constexpr int windowCornerRadius = 4;
    static constexpr int windowBorderThickness = 1;
    static constexpr int separatorWidth = 1;
    static constexpr int keyboardStateHorizontalPadding = 4;
    static constexpr int keyboardStateWidth = 12;
};

export class PromptWindow : public Window
{
protected:
    PromptWindow(
        std::shared_ptr<ILog> log,
        std::shared_ptr<IAutomation> automation,
        std::shared_ptr<ITextBoxLocator> textBoxLocator,
        std::weak_ptr<IObservable<std::optional<TextBoxElement>>> focusedTextBox,
        std::weak_ptr<IObservable<DWORD>> releasedVirtualKeyCode,
        std::weak_ptr<IObservable<HWND>> eventWindow) :
        log(std::move(log)),
        automation(std::move(automation)),
        textBoxLocator(std::move(textBoxLocator)),
        focusedTextBox(std::move(focusedTextBox)),
        textBoxProcessor([this](const auto& value) { OnFocusedTextBoxChanged(value); }),
        virtualKeyCodeProcessor([this](const auto& value) { OnReleasedKeyChanged(value); }),
        eventWindowProcessor([this](const auto& value) { OnEventWindowChanged(value); })
    {
        if (auto textBox = this->focusedTextBox.lock())
        {
            textBoxSubscription = textBox->Subscribe([this](const auto& element)
            {
                textBoxProcessor.Submit(element);
            });
        }
        if (auto virtualKeyCode = releasedVirtualKeyCode.lock())
        {
            releasedVirtualKeyCodeSubscription = virtualKeyCode->Subscribe([this](const auto& code)
            {
                virtualKeyCodeProcessor.Submit(code);
            });
        }
        if (auto window = eventWindow.lock())
        {
            eventWindowSubscription = window->Subscribe([this](const auto& windowHandle)
            {
                eventWindowProcessor.Submit(windowHandle);
            });
        }
    }

    WNDCLASSEXW OnCreateWindowClassType(HINSTANCE instance, const std::wstring& className) override
    {
        constexpr int IDI_APPICON = 1;
        auto windowClass = Window::OnCreateWindowClassType(instance, className);
        windowClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPICON));
        windowClass.hIconSm = (HICON)LoadImage(instance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        windowClass.hbrBackground = nullptr;
        return windowClass;
    }

    HWND OnCreateWindow(HINSTANCE instance, const std::wstring& title, const std::wstring& className) override
    {
        auto dwStyle = WS_POPUP;
        auto dwExStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

        auto handle = CreateWindowExW(
            dwExStyle,
            className.c_str(),
            title.c_str(),
            dwStyle,
            0, 0, 0, 0,
            nullptr,
            nullptr,
            instance,
            this
        );

        return handle;
    }

    void OnInitialized() override
    {
        int width = 0;
        int height = 0;
        GetBounds().GetSize(width, height);

        GraphicsResources resources;
        auto hr = GraphicsInitializer::Create(GetState().GetHandle(), max(width, 1), max(height, 1), resources);
        if (FAILED(hr))
        {
            log->Fatal("Failed to init graphics, hr: " + HResultToString(hr));
            return;
        }

        renderer = std::make_unique<PromptRenderer>(this, resources);
    }

    void OnSizeChanged(UINT width, UINT height) override
    {
        if (width == 0 || height == 0)
            return;

        auto hr = renderer->Resize(width, height);
        if (FAILED(hr))
            log->Error("Failed to resize renderer, hr: " + HResultToString(hr));
    }

private:
    void OnFocusedTextBoxChanged(const std::optional<TextBoxElement>& focusedTextBox)
    {
        if (focusedTextBox.has_value())
        {
            const auto& textBox = focusedTextBox.value();
            UpdateInputStateContent(textBox.WindowHandle, false);
            UpdateWindowPosition(textBox);
            GetState().Show();
        }
        else
        {
            GetState().Hide();
        }
    }

    void OnReleasedKeyChanged(const DWORD& virtualKeyCode)
    {
        if (virtualKeyCode == 0 || !GetState().IsShowing())
            return;

        if (IsLockKeysReleased(virtualKeyCode) || IsInputMethodSwitchKeysReleased(virtualKeyCode))
        {
            if (auto textBox = focusedTextBox.lock())
            {
                const auto& textBoxElement = textBox->Get();
                if (textBoxElement.has_value())
                    UpdateInputStateContent(textBoxElement->WindowHandle, true);
            }
        }
        else if (IsControlKeyReleased(virtualKeyCode))
        {
            if (auto textBox = focusedTextBox.lock())
            {
                const auto& textBoxElement = textBox->Get();
                if (textBoxElement.has_value())
                {
                    bool isFocused = false;
                    auto hr = AutomationOperation::HasKeyboardFocus(textBoxElement.value().Value.get(), isFocused);
                    if (FAILED(hr))
                    {
                        log->Error("Failed to get keyboard focus state, hr: " + HResultToString(hr));
                    }
                    else
                    {
                        if (isFocused)
                            UpdateWindowPosition(textBoxElement.value());
                        else
                            GetState().Hide();
                    }
                }
            }
        }
    }

    void OnEventWindowChanged(const HWND& windowHandle)
    {
        if (windowHandle == nullptr || !GetState().IsShowing())
            return;

        if (auto textBox = focusedTextBox.lock())
        {
            const auto& textBoxElement = textBox->Get();
            if (textBoxElement.has_value())
                UpdateWindowPosition(textBoxElement.value());
        }
    }

    void UpdateInputStateContent(HWND windowHandle, bool isDeferred)
    {
        if (windowHandle == nullptr)
            return;

        if (isDeferred)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

        InputState state;
        auto errorCode = InputMethodDetector::GetInputState(windowHandle, state);
        if (errorCode == ERROR_SUCCESS)
        {
            auto hr = renderer->RenderInputState(state);
            if (FAILED(hr))
                log->Error("Failed to render input state, hr: " + HResultToString(hr));
        }
        else
        {
            log->Error("Failed to get input state, error code: " + std::to_string(errorCode));
        }
    }

    void UpdateWindowPosition(const TextBoxElement& textBoxElement)
    {
        RECT rect;
        if (textBoxLocator->GetPosition(automation.get(), textBoxElement.Value.get(), textBoxElement.Type, rect))
        {
            GetBounds().SetPosition(rect.left, rect.bottom);
        }
        else
        {
            log->Error(std::string("Failed to get position of ").append(GetEnumString(textBoxElement.Type)));
        }
    }

    static bool IsInputMethodSwitchKeysReleased(DWORD virtualKeyCode)
    {
        auto isShiftReleased = virtualKeyCode == VK_SHIFT
            || virtualKeyCode == VK_LSHIFT
            || virtualKeyCode == VK_RSHIFT;
        auto isSpaceReleased = virtualKeyCode == VK_SPACE;

        auto isSpacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        auto isCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0
            || (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0
            || (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
        auto isWinPressed = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;

        return isShiftReleased                     // Shift松开
            || isCtrlPressed && isSpaceReleased    // Ctrl按下+Space松开
            || isCtrlPressed && isSpacePressed     // Ctrl按下+Space按下
            || isWinPressed && isSpaceReleased     // Win按下+Space松开
            || isWinPressed && isSpacePressed;     // Win按下+Space按下
    }

    static bool IsLockKeysReleased(DWORD virtualKeyCode)
    {
        return virtualKeyCode == VK_NUMLOCK || virtualKeyCode == VK_CAPITAL || virtualKeyCode == VK_SCROLL;
    }

    static bool IsControlKeyReleased(DWORD virtualKeyCode)
    {
        return virtualKeyCode == VK_ESCAPE || virtualKeyCode == VK_RETURN;
    }

private:
    std::shared_ptr<ILog> log;
    std::shared_ptr<IAutomation> automation;
    std::shared_ptr<ITextBoxLocator> textBoxLocator;
    std::unique_ptr<PromptRenderer> renderer;
    std::weak_ptr<IObservable<std::optional<TextBoxElement>>> focusedTextBox;
    ConflatingProcessor<std::optional<TextBoxElement>> textBoxProcessor;
    ConflatingProcessor<DWORD> virtualKeyCodeProcessor;
    ConflatingProcessor<HWND> eventWindowProcessor;
    Subscription textBoxSubscription;
    Subscription releasedVirtualKeyCodeSubscription;
    Subscription eventWindowSubscription;
};