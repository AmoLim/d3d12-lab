//
// Created by Amo on 2026/9/26.
//

#ifndef A2WIN32PRACTICE_DX12RENDERER_H
#define A2WIN32PRACTICE_DX12RENDERER_H
#include <array>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>

struct CD3DX12_CPU_DESCRIPTOR_HANDLE;
class CpuDescriptorHeap;
using Microsoft::WRL::ComPtr;

class Dx12Renderer {
public:

    explicit Dx12Renderer(HWND hwnd);

    ~Dx12Renderer() noexcept;

    [[nodiscard]] ID3D12Resource* GetCurrentBackBuffer() const noexcept;

    [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const noexcept;

    [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept;

protected:
    static constexpr UINT SwapChainBufferCount = 2;

private:
    // ===========================================
    // DirectX Initializing
    void Initialize();
    void CreateDevice();
    void CreateFence();
    void CreateCommandFlow();
    void CreateSwapChain();
    void CreateDescriptorHeap();
    void LinkRtvWithResource();
    void CreateDsvResource();
    void SetupViewport();

    // ============================================
    // DirectX Cleaning
    void Shutdown();
    void FlushCommandQueue();

private:
    ComPtr<ID3D12Debug> debugController = nullptr;
    ComPtr<IDXGIFactory4> mdxgiFactory = nullptr;
    ComPtr<ID3D12Device> md3dDevice = nullptr;
    ComPtr<IDXGISwapChain4> mSwapChain = nullptr;
    ComPtr<ID3D12Fence> mFence = nullptr;
    ComPtr<ID3D12GraphicsCommandList6> mCommandList = nullptr;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator = nullptr;
    ComPtr<ID3D12CommandQueue> mCommandQueue = nullptr;
    ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;
    std::array<ComPtr<ID3D12Resource>, SwapChainBufferCount> mSwapChainBuffers;

    std::unique_ptr<CpuDescriptorHeap> mRtvHeap = nullptr;
    std::unique_ptr<CpuDescriptorHeap> mDsvHeap = nullptr;

    D3D12_VIEWPORT mViewport = {};

    UINT64 mCurrentFence = 0;
    UINT mCurrBackBuffer = 0;
    HWND mHwnd = nullptr;

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};


#endif //A2WIN32PRACTICE_DX12RENDERER_H
