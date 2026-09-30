#ifndef A2_WINDOW_CLEAR_TESTS_D3D12_TEST_FIXTURE_H
#define A2_WINDOW_CLEAR_TESTS_D3D12_TEST_FIXTURE_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <gtest/gtest.h>

class D3D12TestFixture : public testing::Test {
protected:
    void SetUp() override {
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        ASSERT_HRESULT_SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())));

        // Use a software adapter so tests do not require a particular GPU.
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        ASSERT_HRESULT_SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(adapter.GetAddressOf())));
        ASSERT_HRESULT_SUCCEEDED(D3D12CreateDevice(
            adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device.GetAddressOf())));
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
};

#endif
