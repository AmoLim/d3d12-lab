//
// Created by Amo on 2026/9/26.
//

#ifndef A2WIN32PRACTICE_DX12RENDERER_H
#define A2WIN32PRACTICE_DX12RENDERER_H
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

class Dx12Renderer {
public:



protected:
    static constexpr UINT SwapChainBufferCount = 2;


private:
    ComPtr<IDXGIFactory6> mdxgiFactory = nullptr;
    ComPtr<ID3D12Device> md3dDevice = nullptr;
    ComPtr<ID3D12Fence> mFence = nullptr;
};


#endif //A2WIN32PRACTICE_DX12RENDERER_H
