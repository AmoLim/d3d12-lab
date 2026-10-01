//
// Created by Amo on 2026/9/26.
//

#include "Renderer/Dx12Renderer.h"

#include <cassert>

#include "Config/WindowConfig.h"
#include "D3D12/CpuDescriptorHeap.h"
#include "D3D12/FenceEvent.h"
#include "Core/Exception.h"


Dx12Renderer::Dx12Renderer(HWND hwnd) : mHwnd(hwnd) {
    Initialize();
}

Dx12Renderer::~Dx12Renderer() noexcept {
    Shutdown();
}

void Dx12Renderer::Shutdown() {
    try {
        FlushCommandQueue();
    } catch (std::exception& e) {
        // TODO: Log exception
        (void) e;
    }
}

ID3D12Resource * Dx12Renderer::GetCurrentBackBuffer() const noexcept {
    assert(mCurrBackBuffer < SwapChainBufferCount);

    return mSwapChainBuffers[mCurrBackBuffer].Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Dx12Renderer::GetCurrentBackBufferView() const noexcept {
    assert(mCurrBackBuffer < SwapChainBufferCount);

    return mRtvHeap->GetCpuHandle(mCurrBackBuffer);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Dx12Renderer::GetDepthStencilView() const noexcept {
    return mDsvHeap->GetCpuHandle(0);
}

void Dx12Renderer::RenderFrame(const FLOAT clearColor[4]) {
    RenderFrameInternal_(clearColor);
}

void Dx12Renderer::RenderFrameInternal_(const FLOAT clearColor[4]) {
}

void Dx12Renderer::Initialize() {
    CreateDevice();
    CreateFence();
    CreateCommandFlow();
    CreateSwapChain();
    CreateDescriptorHeap();
    LinkRtvWithResource();
    CreateDsvResource();
    SetupViewport();
}

void Dx12Renderer::CreateDevice() {
    // 启用debug layer
    UINT factoryFlag = 0;
    ThrowIfFailed(
        D3D12GetDebugInterface(
            IID_PPV_ARGS(&debugController)));
    factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;

    ThrowIfFailed(
        CreateDXGIFactory2(
            factoryFlag,
            IID_PPV_ARGS(mdxgiFactory.GetAddressOf())));

    HRESULT hardwareResult = E_FAIL;
    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter> adapter;
        HRESULT enumResult = mdxgiFactory->EnumAdapters(i, adapter.GetAddressOf());

        if (enumResult == DXGI_ERROR_NOT_FOUND)
            break;
        ThrowIfFailed(enumResult);

        ComPtr<ID3D12Device> device;
        hardwareResult = D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_12_2,
            IID_PPV_ARGS(device.GetAddressOf())
            );

        if (SUCCEEDED(hardwareResult)) {
            ThrowIfFailed(
                device.As(&md3dDevice));
            break;
        }
    }
    ThrowIfFailed(hardwareResult);
}

void Dx12Renderer::CreateFence() {
    mCurrentFence = 0;
    ThrowIfFailed(
        md3dDevice->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(mFence.GetAddressOf())));
}

void Dx12Renderer::CreateCommandFlow() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ThrowIfFailed(
        md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())
            ));

    ThrowIfFailed(
        md3dDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocator.GetAddressOf())
        ));

    ThrowIfFailed(
        md3dDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            mCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(mCommandList.GetAddressOf())
            ));

    ThrowIfFailed(
        mCommandList->Close());
}

void Dx12Renderer::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = static_cast<UINT>(A2WindowClear::WIDTH);
    swapChainDesc.Height = static_cast<UINT>(A2WindowClear::HEIGHT);
    swapChainDesc.Format = mBackBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SwapChainBufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(
        mdxgiFactory->CreateSwapChainForHwnd(
            mCommandQueue.Get(),
            mHwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            swapChain1.GetAddressOf()
            ));

    ThrowIfFailed(swapChain1.As(&mSwapChain));

    mCurrBackBuffer = mSwapChain->GetCurrentBackBufferIndex();
}

void Dx12Renderer::CreateDescriptorHeap() {
    mRtvHeap = std::make_unique<CpuDescriptorHeap>(
        md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, SwapChainBufferCount);
    mDsvHeap = std::make_unique<CpuDescriptorHeap>(
        md3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
}

void Dx12Renderer::LinkRtvWithResource() {
    for (UINT i = 0; i < SwapChainBufferCount; i++) {
        ThrowIfFailed(
            mSwapChain->GetBuffer(
                i, IID_PPV_ARGS(mSwapChainBuffers[i].GetAddressOf())));

        md3dDevice->CreateRenderTargetView(
            mSwapChainBuffers[i].Get(),
            nullptr,
            mRtvHeap->GetCpuHandle(i));
    }
}

void Dx12Renderer::CreateDsvResource() {
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = static_cast<UINT>(A2WindowClear::WIDTH);
    desc.Height = static_cast<UINT>(A2WindowClear::HEIGHT);
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = mDepthStencilFormat;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(
        md3dDevice->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            &optClear,
            IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf()))
        );

    md3dDevice->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        nullptr,
        GetDepthStencilView());
}

void Dx12Renderer::SetupViewport() {
    mViewport.TopLeftX = 0.0f;
    mViewport.TopLeftY = 0.0f;
    mViewport.Width = static_cast<float>(A2WindowClear::WIDTH);
    mViewport.Height = static_cast<float>(A2WindowClear::HEIGHT);
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mCommandList->RSSetViewports(1, &mViewport);
}

void Dx12Renderer::FlushCommandQueue() {
    mCurrentFence++;

    ThrowIfFailed(
        mFence->Signal(mCurrentFence)
        );

    FenceEvent fenceEvent;
    fenceEvent.Wait(mFence.Get(), mCurrentFence);
}
