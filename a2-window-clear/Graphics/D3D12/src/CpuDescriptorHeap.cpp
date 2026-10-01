//
// Created by Amo on 2026/9/27.
//

#include "D3D12/CpuDescriptorHeap.h"

#include <cassert>

#include "Core/Exception.h"

CpuDescriptorHeap::CpuDescriptorHeap(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_TYPE type, const UINT capacity) {
    assert(device != nullptr);
    assert(capacity > 0);
    assert(
        type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV ||
        type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV
    );

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = capacity;
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(
        device->CreateDescriptorHeap(
            &desc,
            IID_PPV_ARGS(mHeap.GetAddressOf())
        )
    );

    mSingleSize =
        device->GetDescriptorHandleIncrementSize(type);


    mCapacity = capacity;
    mType = type;
}

ID3D12DescriptorHeap * CpuDescriptorHeap::Get() const noexcept {
    return mHeap.Get();
}

D3D12_DESCRIPTOR_HEAP_TYPE CpuDescriptorHeap::GetType() const noexcept {
    return mType;
}

UINT CpuDescriptorHeap::GetCapacity() const noexcept {
    return mCapacity;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHeap::GetCpuHandle(const UINT index) const noexcept {
    assert(index < mCapacity);

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(static_cast<INT>(index), mSingleSize);

    return handle;
}
