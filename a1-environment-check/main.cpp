#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

std::string DescribeResult(HRESULT result) {
    std::ostringstream text;
    text << "0x" << std::hex << std::uppercase << std::setfill('0')
         << std::setw(8) << static_cast<std::uint32_t>(result);
    return text.str();
}

void Check(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw std::runtime_error(std::string(operation) + ": " + DescribeResult(result));
    }
}

std::string Utf8(const wchar_t* value) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        throw std::runtime_error("Adapter name conversion failed.");
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}

}  // namespace

int main() {
#if !defined(_DEBUG)
    std::cerr << "[FAIL] A1 requires a Debug build. Select Debug in the CMake profile.\n";
    return 1;
#endif

    try {
        std::cout << "[PASS] Compiler: MSVC " << _MSC_VER << '\n';
        std::cout << "[PASS] Architecture: " << sizeof(void*) * 8 << "-bit\n";
        std::cout << "[PASS] Build configuration: Debug\n";

        ComPtr<IDXGIFactory6> factory;
        Check(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");

        // Enumeration does not create a D3D12 device; debug setup still precedes device creation.
        ComPtr<IDXGIAdapter1> selectedAdapter;
        for (UINT index = 0;; ++index) {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT result = factory->EnumAdapterByGpuPreference(
                index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
            if (result == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            Check(result, "EnumAdapterByGpuPreference");
            DXGI_ADAPTER_DESC1 description{};
            Check(adapter->GetDesc1(&description), "GetDesc1");
            const std::string name = Utf8(description.Description);
            const bool software = (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
            std::cout << "[INFO] Adapter " << index << ": " << name
                      << (software ? " [software]" : " [hardware]") << '\n';
            if (!selectedAdapter && !software && description.VendorId == 0x10DE
                && name.find("RTX 5070 Ti") != std::string::npos) {
                selectedAdapter = adapter;
            }
        }

        if (!selectedAdapter) {
            std::cerr << "[FAIL] RTX 5070 Ti was not found. A1 will not fall back to another GPU.\n";
            return 3;
        }
        std::cout << "[PASS] Selected adapter: NVIDIA GeForce RTX 5070 Ti\n";

        ComPtr<ID3D12Debug> debug;
        const HRESULT debugResult = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        if (FAILED(debugResult)) {
            std::cerr << "[FAIL] D3D12GetDebugInterface: " << DescribeResult(debugResult) << '\n';
            std::cerr << "[ACTION] Check/install the Windows Graphics Tools optional feature, "
                         "then run this Debug build again.\n";
            std::cerr << "[INFO] No D3D12 device was created; this is not an A1 pass.\n";
            return 2;
        }
        debug->EnableDebugLayer();
        std::cout << "[PASS] Debug layer enabled before device creation\n";

        ComPtr<ID3D12Device> device;
        Check(D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_12_0,
                               IID_PPV_ARGS(&device)), "D3D12CreateDevice");
        std::cout << "[PASS] D3D12 device created (minimum feature level 12_0)\n";

        ComPtr<ID3D12InfoQueue> infoQueue;
        Check(device.As(&infoQueue), "Query ID3D12InfoQueue");
        Check(infoQueue->PushEmptyStorageFilter(), "PushEmptyStorageFilter");
        Check(infoQueue->PushEmptyRetrievalFilter(), "PushEmptyRetrievalFilter");
        Check(infoQueue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_INFO,
                                              "A1 controlled debug-queue marker."),
              "AddApplicationMessage");

        bool markerFound = false;
        bool validationProblem = false;
        const UINT64 count = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 index = 0; index < count; ++index) {
            SIZE_T size = 0;
            Check(infoQueue->GetMessage(index, nullptr, &size), "GetMessage size");
            std::vector<std::uint64_t> storage((size + sizeof(std::uint64_t) - 1)
                                               / sizeof(std::uint64_t));
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            Check(infoQueue->GetMessage(index, message, &size), "GetMessage data");
            const std::string description(message->pDescription);
            std::cout << "[DEBUG] " << description << '\n';
            markerFound |= description.find("A1 controlled debug-queue marker.") != std::string::npos;
            validationProblem |= message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING;
        }
        if (!markerFound || validationProblem) {
            std::cerr << "[FAIL] Debug-queue verification needs investigation.\n";
            return 4;
        }
        std::cout << "[PASS] Controlled debug message retrieved; no warnings/errors in this probe\n";
        std::cout << "[PASS] A1 environment probe complete. Official triangle is a separate checkpoint.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 5;
    }
}
