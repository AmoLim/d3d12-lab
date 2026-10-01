#include "D3D12/CpuDescriptorHeap.h"
#include "D3D12TestFixture.h"

#include <initializer_list>
#include <memory>
#include <wrl/implements.h>

// --- Creation ---------------------------------------------------------------
// Check native heap capacity, type and CPU-only flags at capacities 1 and 8.
using CpuDescriptorHeapCreationTest = D3D12TestFixture;

TEST_F(CpuDescriptorHeapCreationTest, CreatesRtvHeapWithRequestedProperties) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, capacity);

        ASSERT_NE(heap.Get(), nullptr);
        // D3D12's read-only getters are not const-qualified.
        const auto desc = const_cast<ID3D12DescriptorHeap*>(heap.Get())->GetDesc();
        EXPECT_EQ(desc.NumDescriptors, capacity);
        EXPECT_EQ(desc.Type, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        EXPECT_EQ(desc.Flags, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    }
}

TEST_F(CpuDescriptorHeapCreationTest, CreatesDsvHeapWithRequestedProperties) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, capacity);

        ASSERT_NE(heap.Get(), nullptr);
        const auto desc = const_cast<ID3D12DescriptorHeap*>(heap.Get())->GetDesc();
        EXPECT_EQ(desc.NumDescriptors, capacity);
        EXPECT_EQ(desc.Type, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        EXPECT_EQ(desc.Flags, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    }
}

// --- Getters ----------------------------------------------------------------
// Check that public capacity and type getters match the constructor arguments.
using CpuDescriptorHeapGetterTest = D3D12TestFixture;

TEST_F(CpuDescriptorHeapGetterTest, ReturnsRequestedRtvProperties) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, capacity);

        EXPECT_EQ(heap.GetCapacity(), capacity);
        EXPECT_EQ(heap.GetType(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
}

TEST_F(CpuDescriptorHeapGetterTest, ReturnsRequestedDsvProperties) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, capacity);

        EXPECT_EQ(heap.GetCapacity(), capacity);
        EXPECT_EQ(heap.GetType(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }
}

// --- CPU Handles ------------------------------------------------------------
// Check start + index * increment for every valid index, including both ends.
using CpuDescriptorHeapHandleTest = D3D12TestFixture;

TEST_F(CpuDescriptorHeapHandleTest, OffsetsEveryRtvHandleByDescriptorSize) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, capacity);

        ASSERT_NE(heap.Get(), nullptr);
        const auto start = const_cast<ID3D12DescriptorHeap*>(heap.Get())->GetCPUDescriptorHandleForHeapStart();
        const UINT increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        ASSERT_GT(increment, 0u);

        for (UINT index = 0; index < capacity; ++index) {
            SCOPED_TRACE(index);
            EXPECT_EQ(heap.GetCpuHandle(index).ptr, start.ptr + static_cast<SIZE_T>(index) * increment);
        }
    }
}

TEST_F(CpuDescriptorHeapHandleTest, OffsetsEveryDsvHandleByDescriptorSize) {
    for (const UINT capacity : {1u, 8u}) {
        SCOPED_TRACE(capacity);
        const CpuDescriptorHeap heap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, capacity);

        ASSERT_NE(heap.Get(), nullptr);
        const auto start = const_cast<ID3D12DescriptorHeap*>(heap.Get())->GetCPUDescriptorHandleForHeapStart();
        const UINT increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        ASSERT_GT(increment, 0u);

        for (UINT index = 0; index < capacity; ++index) {
            SCOPED_TRACE(index);
            EXPECT_EQ(heap.GetCpuHandle(index).ptr, start.ptr + static_cast<SIZE_T>(index) * increment);
        }
    }
}

// --- RAII -------------------------------------------------------------------
// The heap owns a COM marker via private data. Its weak token expires when the
// heap is destroyed, without retaining the heap or inspecting a dangling pointer.
namespace {
    class HeapLifetimeMarker final : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IUnknown> {
    public:
        explicit HeapLifetimeMarker(const std::shared_ptr<int>& token) : token_(token) {}

    private:
        std::shared_ptr<int> token_;
    };

    HRESULT TrackHeapLifetime(ID3D12DescriptorHeap* heap, std::weak_ptr<int>& lifetime) {
        constexpr GUID markerId = {0x09aacf92, 0x7fab, 0x4d70, {0xa3, 0x9c, 0x57, 0x24, 0x40, 0x61, 0xe8, 0x02}};
        const auto token = std::make_shared<int>(0);
        lifetime = token;
        const auto marker = Microsoft::WRL::Make<HeapLifetimeMarker>(token);
        if (!marker) {
            return E_OUTOFMEMORY;
        }

        // SetPrivateDataInterface retains the marker until the heap is destroyed.
        // Local strong references disappear when this helper returns.
        return heap->SetPrivateDataInterface(markerId, marker.Get());
    }

    struct ScopeExitException {};
}

using CpuDescriptorHeapRaiiTest = D3D12TestFixture;

TEST_F(CpuDescriptorHeapRaiiTest, ReleasesNativeHeapWhenLeavingScope) {
    for (const auto type : {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV}) {
        SCOPED_TRACE(type);
        std::weak_ptr<int> lifetime;
        {
            const CpuDescriptorHeap heap(device.Get(), type, 1);
            ASSERT_NE(heap.Get(), nullptr);
            ASSERT_HRESULT_SUCCEEDED(TrackHeapLifetime(heap.Get(), lifetime));
            EXPECT_FALSE(lifetime.expired());
        }

        EXPECT_TRUE(lifetime.expired()) << "The native heap must be released at scope exit";
    }
}

TEST_F(CpuDescriptorHeapRaiiTest, ReleasesNativeHeapDuringExceptionUnwinding) {
    for (const auto type : {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV}) {
        SCOPED_TRACE(type);
        std::weak_ptr<int> lifetime;
        bool caughtExpectedException = false;
        try {
            const CpuDescriptorHeap heap(device.Get(), type, 1);
            ASSERT_NE(heap.Get(), nullptr);
            ASSERT_HRESULT_SUCCEEDED(TrackHeapLifetime(heap.Get(), lifetime));
            EXPECT_FALSE(lifetime.expired());
            throw ScopeExitException{};
        } catch (const ScopeExitException&) {
            caughtExpectedException = true;
        }

        EXPECT_TRUE(caughtExpectedException);
        EXPECT_TRUE(lifetime.expired()) << "The native heap must be released during stack unwinding";
    }
}
