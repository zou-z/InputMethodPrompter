module;
#include <d2d1_3.h>
#include <d2d1svg.h>
export module PromptContentRenderer;

import std;
import Direct2DRenderer;
import SegoeFluentIcons;
import InputMethodDetector;

#define ReturnIfFailed(expression, result) if (FAILED(result = expression)) return result

export class PromptContentRenderer :public Direct2DRenderer
{
private:
    struct SvgIcon
    {
        winrt::com_ptr<ID2D1SvgDocument> Svg;
        int Width;
        int Height;

        SvgIcon(int width, int height)
        {
            Width = width;
            Height = height;
        }
    };

public:
    PromptContentRenderer() :
        keyboardSvgIcon(std::make_unique<SvgIcon>(1024, 704)),
        zhongSvgIcon(std::make_unique<SvgIcon>(832, 960)),
        yingSvgIcon(std::make_unique<SvgIcon>(960, 896))
    {
    }

    HRESULT Initialize(HWND renderTargetHandle, UINT width, UINT height, float scale) override
    {
        auto hr = S_OK;

        ReturnIfFailed(Direct2DRenderer::Initialize(renderTargetHandle, width, height, scale), hr);

        ReturnIfFailed(LoadD2D1SvgDocument(
            SegoeFluentIcons::Keyboard.data(),
            keyboardSvgIcon->Width,
            keyboardSvgIcon->Height,
            keyboardSvgIcon->Svg
        ), hr);

        ReturnIfFailed(LoadD2D1SvgDocument(
            SegoeFluentIcons::Zhong.data(),
            zhongSvgIcon->Width,
            zhongSvgIcon->Height,
            zhongSvgIcon->Svg
        ), hr);

        ReturnIfFailed(LoadD2D1SvgDocument(
            SegoeFluentIcons::Ying.data(),
            yingSvgIcon->Width,
            yingSvgIcon->Height,
            yingSvgIcon->Svg
        ), hr);

        this->width = width;
        this->height = height;
        this->scale = scale;

        return hr;
    }

    HRESULT Resize(UINT width, UINT height, float scale) override
    {
        auto hr = S_OK;

        ReturnIfFailed(Direct2DRenderer::Resize(width, height, scale), hr);

        this->width = width;
        this->height = height;
        this->scale = scale;

        return hr;
    }

    bool SetContent(InputMethodDetector::InputMethodState state)
    {
        if (this->state == state)
            return false;

        this->state = state;
        return true;
    }

    HRESULT Render()
    {
        auto hr = S_OK;

        d2d1DeviceContext->BeginDraw();
        d2d1DeviceContext->Clear(D2D1::ColorF(0, 0, 0, 0));

        winrt::com_ptr<ID2D1SolidColorBrush> backgroundColorBrush;
        winrt::com_ptr<ID2D1SolidColorBrush> borderColorBrush;
        ReturnIfFailed(d2d1DeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), backgroundColorBrush.put()), hr);
        ReturnIfFailed(d2d1DeviceContext->CreateSolidColorBrush(D2D1::ColorF(0.75f, 0.75f, 0.75f), borderColorBrush.put()), hr);

        auto outerRect = GetOuterRect();
        auto innerRect = GetInnerRect(outerRect);
        d2d1DeviceContext->FillRoundedRectangle(outerRect, borderColorBrush.get());
        d2d1DeviceContext->FillRoundedRectangle(innerRect, backgroundColorBrush.get());

        auto iconSvg = GetRenderIconSvg();
        if (iconSvg == nullptr)
            return E_NOTIMPL;

        auto transform = GetSvgIconTransform(iconSvg);
        d2d1DeviceContext->SetTransform(transform);
        d2d1DeviceContext->DrawSvgDocument(iconSvg->Svg.get());
        d2d1DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        ReturnIfFailed(d2d1DeviceContext->EndDraw(), hr);

        ReturnIfFailed(swapChain->Present(1, 0), hr);

        return hr;
    }

private:
    D2D1_ROUNDED_RECT GetOuterRect()
    {
        D2D1_ROUNDED_RECT rect
        {
            .rect = D2D1_RECT_F
            {
                .left = 0.0f,
                .top = 0.0f,
                .right = (float)width,
                .bottom = (float)height,
            },
            .radiusX = outerRadius,
            .radiusY = outerRadius,
        };

        return rect;
    }

    D2D1_ROUNDED_RECT GetInnerRect(const D2D1_ROUNDED_RECT& outerRect)
    {
        D2D1_ROUNDED_RECT rect
        {
            .rect = D2D1_RECT_F
            {
                .left = outerRect.rect.left + borderThickness,
                .top = outerRect.rect.top + borderThickness,
                .right = outerRect.rect.right - borderThickness,
                .bottom = outerRect.rect.bottom - borderThickness,
            },
            .radiusX = innerRadius,
            .radiusY = innerRadius,
        };

        return rect;
    }

    SvgIcon* GetRenderIconSvg()
    {
        switch (state)
        {
        case InputMethodDetector::InputMethodState::EnglishKeyboard:
            return keyboardSvgIcon.get();
        case InputMethodDetector::InputMethodState::EnglishInputMode:
            return yingSvgIcon.get();
        case InputMethodDetector::InputMethodState::NativeInputMode:
            return zhongSvgIcon.get();
        default:
            return nullptr;
        }
    }

    D2D1::Matrix3x2F GetSvgIconTransform(const SvgIcon* svgIcon) const
    {
        auto svgIconRenderSize = svgIconSize * scale;
        auto scaleX = svgIconRenderSize / svgIcon->Width;
        auto scaleY = svgIconRenderSize / svgIcon->Height;

        auto scale = min(scaleX, scaleY);
        auto x = (width - svgIcon->Width * scale) / 2;
        auto y = (height - svgIcon->Height * scale) / 2;

        return D2D1::Matrix3x2F::Scale(scale, scale) * D2D1::Matrix3x2F::Translation(x, y);
    }

private:
    static constexpr auto borderThickness = 1.0f;
    static constexpr auto outerRadius = 6.0f;
    static constexpr auto innerRadius = outerRadius - borderThickness;
    static constexpr auto svgIconSize = 16;
    InputMethodDetector::InputMethodState state;
    std::unique_ptr<SvgIcon> keyboardSvgIcon;
    std::unique_ptr<SvgIcon> zhongSvgIcon;
    std::unique_ptr<SvgIcon> yingSvgIcon;
    UINT width;
    UINT height;
    float scale;
};
