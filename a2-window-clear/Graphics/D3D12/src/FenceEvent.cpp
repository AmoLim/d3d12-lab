//
// Created by Amo on 2026/9/30.
//

#include "D3D12/FenceEvent.h"

#include <cassert>
#include <utility>

#include "Core/Exception.h"

FenceEvent::FenceEvent() {
    mHandle = CreateEvent(nullptr, false, false, nullptr);

    ThrowIfFailedWin32(
        mHandle != nullptr,
        "CreateEvent() failed");
}

FenceEvent::~FenceEvent() {
    if (mHandle != nullptr) {
        CloseHandle(mHandle);
    }
}

FenceEvent::FenceEvent(FenceEvent &&other) noexcept : mHandle(std::exchange(other.mHandle, nullptr)) {
}

FenceEvent & FenceEvent::operator=(FenceEvent &&other) noexcept {
    if (this != &other) {
        if (mHandle != nullptr) {
            CloseHandle(mHandle);
        }

        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }

    return *this;
}

void FenceEvent::Wait(ID3D12Fence *fence, const UINT64 fenceValue) const {
    assert(fence != nullptr);

    if (fence->GetCompletedValue() < fenceValue) {
        ThrowIfFailed(
            fence->SetEventOnCompletion(fenceValue, mHandle)
            );

        const DWORD result = WaitForSingleObject(mHandle, INFINITE);

        ThrowIfFailedWin32(
            result == WAIT_OBJECT_0,
            "WaitForSingleObject() failed"
        )
    }
}
