module;
//#include <d2d1svg.h>
//#include <Shlwapi.h>
#include "Macros.h"
#include "GraphicsTypes.h"
#include <dwrite.h>
export module RenderComponent;


export struct RectF
{
    float Left;
    float Top;
    float Width;
    float Height;

    RectF(float left, float top, float width, float height)
        : Left(left), Top(top), Width(width), Height(height)
    {
    }

    float GetRight() const { return Left + Width; }

    float GetBottom() const { return Top + Height; }

    D2D1_RECT_F ToD2D1RectF() const
    {
        return D2D1::RectF(Left, Top, Left + Width, Top + Height);
    }
};

namespace GraphicsInitializer
{
    static HRESULT GetDxgiDevice(
        const winrt::com_ptr<ID3D11Device>& d3d11Device,
        winrt::com_ptr<IDXGIDevice>& dxgiDevice)
    {
        dxgiDevice = d3d11Device.try_as<IDXGIDevice>();
        return dxgiDevice == nullptr ? E_POINTER : S_OK;
    }

    static HRESULT InitD3D11Device(
        winrt::com_ptr<ID3D11Device>& d3d11Device,
        winrt::com_ptr<ID3D11DeviceContext>& d3d11DeviceContext)
    {
        return D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            d3d11Device.put(),
            nullptr,
            d3d11DeviceContext.put()
        );
    }

    static HRESULT InitD2D1Device(
        const winrt::com_ptr<IDXGIDevice>& dxgiDevice,
        winrt::com_ptr<ID2D1Factory6>& d2d1Factory,
        winrt::com_ptr<ID2D1Device5>& d2d1Device,
        winrt::com_ptr<ID2D1DeviceContext5>& d2d1DeviceContext)
    {
        auto hr = S_OK;
        D2D1_FACTORY_OPTIONS factoryOptions{};
        RETURN_IF_FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factoryOptions, d2d1Factory.put()), hr);
        RETURN_IF_FAILED(d2d1Factory->CreateDevice(dxgiDevice.get(), d2d1Device.put()), hr);
        RETURN_IF_FAILED(d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d1DeviceContext.put()), hr);
        return hr;
    }

    static HRESULT InitSwapChain(
        const winrt::com_ptr<IDXGIDevice>& dxgiDevice,
        const winrt::com_ptr<ID3D11Device>& d3d11Device,
        winrt::com_ptr<IDXGISwapChain1>& swapChain,
        UINT width,
        UINT height)
    {
        auto hr = S_OK;

        winrt::com_ptr<IDXGIAdapter> adapter;
        RETURN_IF_FAILED(hr = dxgiDevice->GetAdapter(adapter.put()), hr);

        winrt::com_ptr<IDXGIFactory2> factory;
        RETURN_IF_FAILED(adapter->GetParent(IID_PPV_ARGS(factory.put())), hr);

        DXGI_SWAP_CHAIN_DESC1 descriptor
        {
            .Width = width,
            .Height = height,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .SampleDesc = DXGI_SAMPLE_DESC
            {
                .Count = 1,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            .AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED,
        };

        return factory->CreateSwapChainForComposition(d3d11Device.get(), &descriptor, nullptr, swapChain.put());
    }

    static HRESULT InitComposition(
        const winrt::com_ptr<IDXGIDevice>& dxgiDevice,
        const winrt::com_ptr<IDXGISwapChain1>& swapChain,
        winrt::com_ptr<IDCompositionDevice>& compositionDevice,
        winrt::com_ptr<IDCompositionTarget>& compositionTarget,
        winrt::com_ptr<IDCompositionVisual>& compositionVisual,
        HWND targetHandle)
    {
        auto hr = S_OK;

        RETURN_IF_FAILED(DCompositionCreateDevice(dxgiDevice.get(), IID_PPV_ARGS(compositionDevice.put())), hr);

        RETURN_IF_FAILED(compositionDevice->CreateTargetForHwnd(targetHandle, TRUE, compositionTarget.put()), hr);
        RETURN_IF_FAILED(compositionDevice->CreateVisual(compositionVisual.put()), hr);
        RETURN_IF_FAILED(compositionVisual->SetContent(swapChain.get()), hr);
        RETURN_IF_FAILED(compositionTarget->SetRoot(compositionVisual.get()), hr);

        return compositionDevice->Commit();
    }

    static HRESULT InitD2D1Bitmap(
        const winrt::com_ptr<IDXGISwapChain1>& swapChain,
        const winrt::com_ptr<ID2D1DeviceContext5>& d2d1DeviceContext,
        winrt::com_ptr<ID2D1Bitmap1>& d2d1Bitmap)
    {
        auto hr = S_OK;

        winrt::com_ptr<IDXGISurface> surface;
        RETURN_IF_FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(surface.put())), hr);

        auto bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED
            )
        );

        return d2d1DeviceContext->CreateBitmapFromDxgiSurface(surface.get(), &bitmapProperties, d2d1Bitmap.put());
    }

    export HRESULT Create(HWND renderTarget, UINT width, UINT height, GraphicsResources& resources)
    {
        HRESULT hr = S_OK;
        winrt::com_ptr<IDXGIDevice> dxgiDevice;

        RETURN_IF_FAILED(InitD3D11Device(resources.D3D11Device, resources.D3D11DeviceContext), hr);
        RETURN_IF_FAILED(GetDxgiDevice(resources.D3D11Device, dxgiDevice), hr);
        RETURN_IF_FAILED(InitD2D1Device(dxgiDevice, resources.D2D1Factory, resources.D2D1Device, resources.D2D1DeviceContext), hr);
        RETURN_IF_FAILED(InitSwapChain(dxgiDevice, resources.D3D11Device, resources.SwapChain, width, height), hr);
        RETURN_IF_FAILED(
            InitComposition(
                dxgiDevice,
                resources.SwapChain,
                resources.CompositionDevice,
                resources.CompositionTarget,
                resources.CompositionVisual,
                renderTarget),
            hr
        );
        RETURN_IF_FAILED(InitD2D1Bitmap(resources.SwapChain, resources.D2D1DeviceContext, resources.D2D1Bitmap), hr);

        resources.D2D1DeviceContext->SetTarget(resources.D2D1Bitmap.get());

        return hr;
    }
}

namespace GraphicsUtilities
{
    static HRESULT ResizeSwapChainBuffers(
        const winrt::com_ptr<ID3D11DeviceContext>& d3d11DeviceContext,
        const winrt::com_ptr<IDXGISwapChain1>& swapChain,
        UINT width,
        UINT height)
    {
        d3d11DeviceContext->ClearState();
        d3d11DeviceContext->Flush();

        return swapChain->ResizeBuffers(
            0,                    // 表示保持原来的数量
            width,
            height,
            DXGI_FORMAT_UNKNOWN,  // 保持原格式
            0
        );
    }

    export HRESULT Resize(
        const winrt::com_ptr<ID3D11DeviceContext>& d3d11DeviceContext,
        const winrt::com_ptr<ID2D1DeviceContext5>& d2d1DeviceContext,
        const winrt::com_ptr<IDXGISwapChain1>& swapChain,
        winrt::com_ptr<ID2D1Bitmap1>& d2d1Bitmap,
        UINT width,
        UINT height)
    {
        HRESULT hr = S_OK;

        d2d1DeviceContext->SetTarget(nullptr);
        d2d1Bitmap = nullptr;

        RETURN_IF_FAILED(ResizeSwapChainBuffers(d3d11DeviceContext, swapChain, width, height), hr);
        RETURN_IF_FAILED(GraphicsInitializer::InitD2D1Bitmap(swapChain, d2d1DeviceContext, d2d1Bitmap), hr);

        d2d1DeviceContext->SetTarget(d2d1Bitmap.get());

        return hr;
    }
}

namespace GraphicsRender
{
    export void BeginDraw(const GraphicsResources& resources, const D2D1_COLOR_F& clearColor)
    {
        resources.D2D1DeviceContext->BeginDraw();
        resources.D2D1DeviceContext->Clear(clearColor);
    }

    export HRESULT EndDraw(const GraphicsResources& resources)
    {
        HRESULT hr = S_OK;
        RETURN_IF_FAILED(resources.D2D1DeviceContext->EndDraw(), hr);
        RETURN_IF_FAILED(resources.SwapChain->Present(1, 0), hr);
        return hr;
    }

    export HRESULT DrawBorder(
        const GraphicsResources& resources,
        const RectF& position,
        float cornerRadius,
        float borderThickness,
        const D2D1_COLOR_F& borderColor,
        const D2D1_COLOR_F& backgroundColor)
    {
        auto innerRadius = cornerRadius - borderThickness;

        D2D1_ROUNDED_RECT outerRect(D2D1::RoundedRect(position.ToD2D1RectF(), cornerRadius, cornerRadius));
        D2D1_ROUNDED_RECT innerRect(D2D1::RoundedRect(
            D2D1::RectF(
                position.Left + borderThickness,
                position.Top + borderThickness,
                position.GetRight() - borderThickness,
                position.GetBottom() - borderThickness
            ),
            innerRadius,
            innerRadius
        ));

        HRESULT hr = S_OK;
        winrt::com_ptr<ID2D1SolidColorBrush> borderColorBrush;
        winrt::com_ptr<ID2D1SolidColorBrush> backgroundColorBrush;
        RETURN_IF_FAILED(resources.D2D1DeviceContext->CreateSolidColorBrush(borderColor, borderColorBrush.put()), hr);
        RETURN_IF_FAILED(resources.D2D1DeviceContext->CreateSolidColorBrush(backgroundColor, backgroundColorBrush.put()), hr);

        resources.D2D1DeviceContext->FillRoundedRectangle(outerRect, borderColorBrush.get());
        resources.D2D1DeviceContext->FillRoundedRectangle(innerRect, backgroundColorBrush.get());

        return hr;
    }

    export HRESULT DrawBorder(
        const GraphicsResources& resources,
        const RectF& position,
        float cornerRadius,
        const D2D1_COLOR_F& backgroundColor)
    {
        D2D1_ROUNDED_RECT rect(D2D1::RoundedRect(position.ToD2D1RectF(), cornerRadius, cornerRadius));

        HRESULT hr = S_OK;
        winrt::com_ptr<ID2D1SolidColorBrush> backgroundColorBrush;
        RETURN_IF_FAILED(resources.D2D1DeviceContext->CreateSolidColorBrush(backgroundColor, backgroundColorBrush.put()), hr);

        resources.D2D1DeviceContext->FillRoundedRectangle(rect, backgroundColorBrush.get());

        return hr;
    }

    export HRESULT DrawString(
        const GraphicsResources& resources,
        const std::wstring& text,
        const std::wstring& fontName,
        float fontSize,
        const D2D1_COLOR_F& fontColor,
        const RectF& textRect,
        DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
    {
        HRESULT hr = S_OK;

        winrt::com_ptr<IDWriteFactory> dwriteFactory;
        RETURN_IF_FAILED(
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(dwriteFactory.put())),
            hr
        );

        winrt::com_ptr<IDWriteTextFormat> textFormat;
        RETURN_IF_FAILED(
            dwriteFactory->CreateTextFormat(
                fontName.c_str(),
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize,
                L"en-us",
                textFormat.put()),
            hr
        );

        RETURN_IF_FAILED(textFormat->SetTextAlignment(textAlignment), hr);
        RETURN_IF_FAILED(textFormat->SetParagraphAlignment(paragraphAlignment), hr);

        winrt::com_ptr<ID2D1SolidColorBrush> fontColorBrush;
        RETURN_IF_FAILED(resources.D2D1DeviceContext->CreateSolidColorBrush(fontColor, fontColorBrush.put()), hr);

        auto rect = textRect.ToD2D1RectF();
        resources.D2D1DeviceContext->DrawText(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            textFormat.get(),
            &rect,
            fontColorBrush.get(),
            D2D1_DRAW_TEXT_OPTIONS_NONE,
            DWRITE_MEASURING_MODE_NATURAL
        );

        return hr;
    }

    export HRESULT FillRectangle(
        const GraphicsResources& resources,
        const RectF& position,
        const D2D1_COLOR_F& fillColor)
    {
        HRESULT hr = S_OK;

        winrt::com_ptr<ID2D1SolidColorBrush> fillColorBrush;
        RETURN_IF_FAILED(resources.D2D1DeviceContext->CreateSolidColorBrush(fillColor, fillColorBrush.put()), hr);

        auto rect = position.ToD2D1RectF();
        resources.D2D1DeviceContext->FillRectangle(&rect, fillColorBrush.get());

        return hr;
    }
}