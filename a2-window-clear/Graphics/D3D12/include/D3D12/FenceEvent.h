//
// Created by Amo on 2026/9/30.
//

#ifndef A2_WINDOW_CLEAR_FENCEEVENT_H
#define A2_WINDOW_CLEAR_FENCEEVENT_H

#include <Windows.h>
#include <d3d12.h>

class FenceEvent {
public:
    FenceEvent();

    ~FenceEvent();

    FenceEvent(const FenceEvent &other) = delete;

    FenceEvent(FenceEvent &&other) noexcept;

    FenceEvent & operator=(const FenceEvent &other) = delete;

    FenceEvent & operator=(FenceEvent &&other) noexcept;

    void Wait(ID3D12Fence* fence, UINT64 fenceValue) const;

private:
    HANDLE mHandle = nullptr;
};


#endif //A2_WINDOW_CLEAR_FENCEEVENT_H
