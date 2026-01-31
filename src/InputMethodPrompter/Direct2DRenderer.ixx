module;
#include <winrt/base.h>
#include <Windows.h>
#include <d3d11.h>
#include <d2d1_3.h>
#include <d2d1svg.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <Shlwapi.h>
export module Direct2DRenderer;

#define ReturnIfFailed(expression, result) if (FAILED(result = expression)) return result

export class Direct2DRenderer
{
public:
    virtual HRESULT Initialize(HWND renderTargetHandle, UINT width, UINT height, float scale)
    {
        auto hr = S_OK;

        ReturnIfFailed(InitD3D11Device(), hr);

        auto dxgiDevice = d3d11Device.try_as<IDXGIDevice>();
        if (dxgiDevice == nullptr)
            return E_POINTER;

        ReturnIfFailed(InitD2D1Device(dxgiDevice.get()), hr);

        ReturnIfFailed(InitSwapChain(dxgiDevice.get(), width, height), hr);

        ReturnIfFailed(InitComposition(dxgiDevice.get(), renderTargetHandle), hr);

        ReturnIfFailed(InitD2D1Bitmap(), hr);

        d2d1DeviceContext->SetTarget(d2d1Bitmap.get());

        return hr;
    }

    virtual HRESULT Resize(UINT width, UINT height, float scale)
    {
        auto hr = S_OK;

        d2d1DeviceContext->SetTarget(nullptr);
        d2d1Bitmap = nullptr;

        ReturnIfFailed(ResizeSwapChainBuffers(width, height), hr);

        ReturnIfFailed(InitD2D1Bitmap(), hr);

        d2d1DeviceContext->SetTarget(d2d1Bitmap.get());

        return hr;
    }

protected:
    HRESULT LoadD2D1SvgDocument(const char* data, UINT width, UINT height, winrt::com_ptr<ID2D1SvgDocument>& svgDocument)
    {
        auto stream = CreateStream(data);
        if (stream == nullptr)
            return E_POINTER;

        auto viewportSize = D2D1::SizeF((float)width, (float)height);

        return d2d1DeviceContext->CreateSvgDocument(
            stream.get(),
            viewportSize,
            svgDocument.put()
        );
    }

private:
    HRESULT InitD3D11Device()
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

    HRESULT InitD2D1Device(IDXGIDevice* dxgiDevice)
    {
        auto hr = S_OK;

        D2D1_FACTORY_OPTIONS factoryOptions{};
        ReturnIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factoryOptions, d2d1Factory.put()), hr);

        ReturnIfFailed(d2d1Factory->CreateDevice(dxgiDevice, d2d1Device.put()), hr);

        ReturnIfFailed(d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d1DeviceContext.put()), hr);

        return hr;
    }

    HRESULT InitSwapChain(IDXGIDevice* dxgiDevice, UINT width, UINT height)
    {
        auto hr = S_OK;

        winrt::com_ptr<IDXGIAdapter> adapter;
        ReturnIfFailed(hr = dxgiDevice->GetAdapter(adapter.put()), hr);

        winrt::com_ptr<IDXGIFactory2> factory;
        ReturnIfFailed(adapter->GetParent(IID_PPV_ARGS(factory.put())), hr);

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

    HRESULT InitComposition(IDXGIDevice* dxgiDevice, HWND targetHandle)
    {
        auto hr = S_OK;

        ReturnIfFailed(DCompositionCreateDevice(dxgiDevice, IID_PPV_ARGS(compositionDevice.put())), hr);

        ReturnIfFailed(compositionDevice->CreateTargetForHwnd(targetHandle, TRUE, compositionTarget.put()), hr);
        ReturnIfFailed(compositionDevice->CreateVisual(compositionVisual.put()), hr);
        ReturnIfFailed(compositionVisual->SetContent(swapChain.get()), hr);
        ReturnIfFailed(compositionTarget->SetRoot(compositionVisual.get()), hr);

        return compositionDevice->Commit();
    }

    HRESULT InitD2D1Bitmap()
    {
        auto hr = S_OK;

        winrt::com_ptr<IDXGISurface> surface;
        ReturnIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(surface.put())), hr);

        auto bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED
            )
        );

        return d2d1DeviceContext->CreateBitmapFromDxgiSurface(surface.get(), &bitmapProperties, d2d1Bitmap.put());
    }

    HRESULT ResizeSwapChainBuffers(UINT width, UINT height)
    {
        d3d11DeviceContext->ClearState();
        d3d11DeviceContext->Flush();

        return swapChain->ResizeBuffers(
            0,  // 表示保持原来的数量
            width,
            height,
            DXGI_FORMAT_UNKNOWN,  // 保持原格式
            0
        );
    }

    static winrt::com_ptr<IStream> CreateStream(const char* data)
    {
        winrt::com_ptr<IStream> stream;
        stream.copy_from(SHCreateMemStream(reinterpret_cast<const BYTE*>(data), (UINT)strlen(data)));
        return stream;
    }

protected:
    winrt::com_ptr<ID3D11Device> d3d11Device;
    winrt::com_ptr<ID3D11DeviceContext> d3d11DeviceContext;

    winrt::com_ptr<ID2D1Factory6> d2d1Factory;
    winrt::com_ptr<ID2D1Device5> d2d1Device;
    winrt::com_ptr<ID2D1DeviceContext5> d2d1DeviceContext;

    winrt::com_ptr<IDXGISwapChain1> swapChain;

    winrt::com_ptr<IDCompositionDevice> compositionDevice;
    winrt::com_ptr<IDCompositionTarget> compositionTarget;
    winrt::com_ptr<IDCompositionVisual> compositionVisual;

    winrt::com_ptr<ID2D1Bitmap1> d2d1Bitmap;
};
