#pragma once
#include <winrt/base.h>
#include <d3d11.h>
#include <d2d1_3.h>
#include <dxgi1_2.h>
#include <dcomp.h>


// 在头文件中定义，解决 IntelliSense E0833 误报问题
struct GraphicsResources
{
    winrt::com_ptr<ID3D11Device> D3D11Device;
    winrt::com_ptr<ID3D11DeviceContext> D3D11DeviceContext;

    winrt::com_ptr<ID2D1Factory6> D2D1Factory;
    winrt::com_ptr<ID2D1Device5> D2D1Device;
    winrt::com_ptr<ID2D1DeviceContext5> D2D1DeviceContext;

    winrt::com_ptr<IDXGISwapChain1> SwapChain;

    winrt::com_ptr<IDCompositionDevice> CompositionDevice;
    winrt::com_ptr<IDCompositionTarget> CompositionTarget;
    winrt::com_ptr<IDCompositionVisual> CompositionVisual;

    winrt::com_ptr<ID2D1Bitmap1> D2D1Bitmap;
};