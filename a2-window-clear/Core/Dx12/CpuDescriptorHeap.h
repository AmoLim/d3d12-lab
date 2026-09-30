//
// Created by Amo on 2026/9/27.
//

#ifndef A2_WINDOW_CLEAR_CPUDESCRIPTORHEAP_H
#define A2_WINDOW_CLEAR_CPUDESCRIPTORHEAP_H

#include <d3d12.h>
#include <wrl/client.h>
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

class CpuDescriptorHeap {
public:
    explicit CpuDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity);

    [[nodiscard]] ID3D12DescriptorHeap* Get() const noexcept;

    [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetType() const noexcept;

    [[nodiscard]] UINT GetCapacity() const noexcept;

    [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const noexcept;

private:
    ComPtr<ID3D12DescriptorHeap> mHeap = nullptr;
    UINT mSingleSize = 0;
    UINT mCapacity = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE mType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
};


#endif //A2_WINDOW_CLEAR_CPUDESCRIPTORHEAP_H
