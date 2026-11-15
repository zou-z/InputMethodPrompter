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
import EditorRecognizer;
import AutomationComponent;
import TextBoxService;
import EnumComponent;

using namespace Reactive;

export enum class InputMethodState
{
    Unknown,
    EnglishKeyboard,
    EnglishInputMode,
    ChineseInputMode,
};

struct KeyboardState
{
    bool IsNumLocked;
    bool IsCapsLocked;
    bool IsScrollLocked;
};

struct InputState
{
    InputMethodState MethodState;
    KeyboardState KeyboardState;
};

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

    HRESULT SetInputMethodState(InputMethodState value)
    {
        RETURN_IF(state.MethodState == value, S_OK);
        state.MethodState = value;
        return Render();
    }

    HRESULT SetIsNumLocked(bool value)
    {
        RETURN_IF(state.KeyboardState.IsNumLocked == value, S_OK);
        state.KeyboardState.IsNumLocked = value;
        return Render();
    }

    HRESULT SetIsCapsLocked(bool value)
    {
        RETURN_IF(state.KeyboardState.IsCapsLocked == value, S_OK);
        state.KeyboardState.IsCapsLocked = value;
        return Render();
    }

    HRESULT SetIsScrollLocked(bool value)
    {
        RETURN_IF(state.KeyboardState.IsScrollLocked == value, S_OK);
        state.KeyboardState.IsScrollLocked = value;
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
        std::weak_ptr<IObservable<std::optional<TextBoxElement>>> focusedTextBox) :
        log(std::move(log)),
        automation(std::move(automation)),
        textBoxLocator(std::move(textBoxLocator))
    {
        if (auto textBox = focusedTextBox.lock())
        {
            textBoxSubscription = textBox->Subscribe([this](const auto& element)
            {
                OnFocusedTextBoxChanged(element);
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
            0,
            0,
            0,
            0,
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
#pragma region MyRegion
        auto hr = renderer->SetInputMethodState(InputMethodState::ChineseInputMode);
#pragma endregion

        // 按下按键
        // 窗口或输入框焦点变更
        // 窗口最大化/最小化

        if (focusedTextBox.has_value())
        {
            RECT rect;
            const auto& textBox = focusedTextBox.value();
            if (textBoxLocator->GetPosition(automation.get(), textBox.Value.get(), textBox.Type, rect))
            {
                GetBounds().SetPosition(rect.left, rect.bottom);
            }
            else
            {
                log->Error(std::string("Failed to get position of ").append(GetEnumString(textBox.Type)));
            }

            GetState().Show();
        }
        else
        {
            GetState().Hide();
        }
    }

private:
    std::shared_ptr<ILog> log;
    std::shared_ptr<IAutomation> automation;
    std::shared_ptr<ITextBoxLocator> textBoxLocator;
    std::unique_ptr<PromptRenderer> renderer;
    Subscription textBoxSubscription;
    EditorRecognizerService editorRecognizerService;
};