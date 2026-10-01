#include "Config/WindowConfig.h"
#include "Renderer/Dx12Renderer.h"
#include "Core/DxException.h"

#include <d3dx12.h>
#include <gtest/gtest.h>
#include <wrl/implements.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    class HiddenWindow {
    public:
        HiddenWindow() {
            constexpr DWORD style = WS_OVERLAPPEDWINDOW;
            RECT bounds{0, 0, A2WindowClear::WIDTH, A2WindowClear::HEIGHT};
            if (!AdjustWindowRect(&bounds, style, FALSE)) {
                throw std::runtime_error("Test setup: AdjustWindowRect failed");
            }
            hwnd_ = CreateWindowExW(
                0, L"STATIC", L"Dx12Renderer test", style,
                CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
                nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (!hwnd_) {
                throw std::runtime_error("Test setup: CreateWindowExW failed");
            }
        }

        ~HiddenWindow() { DestroyWindow(hwnd_); }
        HiddenWindow(const HiddenWindow&) = delete;
        HiddenWindow& operator=(const HiddenWindow&) = delete;

        HWND Get() const { return hwnd_; }

    private:
        HWND hwnd_ = nullptr;
    };

    // Isolate access violations while still requiring successful exit, not a crash.
    // Propagate GoogleTest failures explicitly: EXPECT_EXIT alone ignores them.
    template <typename Scenario>
    [[noreturn]] void RunRendererScenario(Scenario scenario) {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        try {
            {
                HiddenWindow window;
                scenario(window.Get());
            }
            if (testing::Test::HasFailure()) {
                std::cerr << "Renderer scenario assertions failed\n";
                std::exit(EXIT_FAILURE);
            }
            std::exit(EXIT_SUCCESS);
        } catch (const DxException& error) {
            std::cerr << "Renderer scenario HRESULT: 0x" << std::hex
                      << static_cast<unsigned long>(error.ErrorCode()) << ", " << error.what() << '\n';
            std::exit(EXIT_FAILURE);
        } catch (const std::exception& error) {
            std::cerr << "Renderer scenario exception: " << error.what() << '\n';
            std::exit(EXIT_FAILURE);
        } catch (...) {
            std::cerr << "Renderer scenario threw an unknown exception\n";
            std::exit(EXIT_FAILURE);
        }
    }

    ID3D12Resource* RequireBackBuffer(const Dx12Renderer& renderer) {
        auto* resource = renderer.GetCurrentBackBuffer();
        if (!resource) {
            // Exit this child immediately so a broken destructor cannot hide the
            // initialization failure. Dedicated lifecycle tests exercise destruction.
            std::cerr << "Constructor did not create a back buffer (Ready invariant I1)\n";
            std::exit(EXIT_FAILURE);
        }
        return resource;
    }

    class ResourceLifetimeMarker final : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IUnknown> {
    public:
        explicit ResourceLifetimeMarker(const std::shared_ptr<int>& token) : token_(token) {}

    private:
        std::shared_ptr<int> token_;
    };

    HRESULT TrackResourceLifetime(ID3D12Resource* resource, std::weak_ptr<int>& lifetime) {
        constexpr GUID markerId = {0x0bc125ab, 0x8261, 0x473a, {0x93, 0xe7, 0x5d, 0xe1, 0x32, 0xb3, 0x7a, 0x86}};
        const auto token = std::make_shared<int>(0);
        lifetime = token;
        const auto marker = Microsoft::WRL::Make<ResourceLifetimeMarker>(token);
        if (!marker) {
            return E_OUTOFMEMORY;
        }
        return resource->SetPrivateDataInterface(markerId, marker.Get());
    }

    struct ScopeExitException {};
}

// --- Initialization ---------------------------------------------------------
// A valid hidden window must produce a Ready renderer through its public API.
TEST(Dx12RendererDeathTest, ConstructorProvidesStableBackBuffer) {
    const auto scenario = [](HWND hwnd) {
        const Dx12Renderer renderer(hwnd);
        auto* resource = RequireBackBuffer(renderer);
        EXPECT_EQ(renderer.GetCurrentBackBuffer(), resource);
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(Dx12RendererDeathTest, BackBufferMatchesConfiguredSizeAndFormat) {
    const auto scenario = [](HWND hwnd) {
        const Dx12Renderer renderer(hwnd);
        const auto desc = RequireBackBuffer(renderer)->GetDesc();
        EXPECT_EQ(desc.Dimension, D3D12_RESOURCE_DIMENSION_TEXTURE2D);
        EXPECT_EQ(desc.Width, static_cast<UINT64>(A2WindowClear::WIDTH));
        EXPECT_EQ(desc.Height, static_cast<UINT>(A2WindowClear::HEIGHT));
        EXPECT_EQ(desc.Format, DXGI_FORMAT_R8G8B8A8_UNORM);
        EXPECT_EQ(desc.SampleDesc.Count, 1u);
        EXPECT_NE(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

// --- Descriptor Getters ------------------------------------------------------
// Without a frame advance, repeated calls must return the same nonzero handle.
TEST(Dx12RendererDeathTest, CurrentRtvHandleIsValidAndStable) {
    const auto scenario = [](HWND hwnd) {
        const Dx12Renderer renderer(hwnd);
        RequireBackBuffer(renderer);
        const auto handle = renderer.GetCurrentBackBufferView();
        EXPECT_NE(handle.ptr, SIZE_T{0});
        EXPECT_EQ(renderer.GetCurrentBackBufferView().ptr, handle.ptr);
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(Dx12RendererDeathTest, DepthStencilHandleIsValidAndStable) {
    const auto scenario = [](HWND hwnd) {
        const Dx12Renderer renderer(hwnd);
        RequireBackBuffer(renderer);
        const auto handle = renderer.GetDepthStencilView();
        EXPECT_NE(handle.ptr, SIZE_T{0});
        EXPECT_EQ(renderer.GetDepthStencilView().ptr, handle.ptr);
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

// --- Lifetime ---------------------------------------------------------------
// Destroy the renderer before the window; repeat initialization on the same HWND.
TEST(Dx12RendererDeathTest, DestructionCompletesBeforeWindowDestruction) {
    const auto scenario = [](HWND hwnd) {
        {
            const Dx12Renderer renderer(hwnd);
        }
        EXPECT_TRUE(IsWindow(hwnd));
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(Dx12RendererDeathTest, CanRecreateRendererForSameWindow) {
    const auto scenario = [](HWND hwnd) {
        for (int iteration = 0; iteration < 2; ++iteration) {
            SCOPED_TRACE(iteration);
            const Dx12Renderer renderer(hwnd);
            RequireBackBuffer(renderer);
        }
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

// --- RAII -------------------------------------------------------------------
// Observe resource release without retaining it, and exercise exception unwinding.
TEST(Dx12RendererDeathTest, ReleasesOwnedBackBufferAtScopeExit) {
    const auto scenario = [](HWND hwnd) {
        std::weak_ptr<int> lifetime;
        {
            const Dx12Renderer renderer(hwnd);
            auto* resource = RequireBackBuffer(renderer);
            ASSERT_HRESULT_SUCCEEDED(TrackResourceLifetime(resource, lifetime));
            EXPECT_FALSE(lifetime.expired());
        }
        EXPECT_TRUE(lifetime.expired());
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}

TEST(Dx12RendererDeathTest, DestructionIsSafeDuringExceptionUnwinding) {
    const auto scenario = [](HWND hwnd) {
        bool caughtExpectedException = false;
        try {
            const Dx12Renderer renderer(hwnd);
            throw ScopeExitException{};
        } catch (const ScopeExitException&) {
            caughtExpectedException = true;
        }
        EXPECT_TRUE(caughtExpectedException);
        EXPECT_TRUE(IsWindow(hwnd));
    };
    EXPECT_EXIT(RunRendererScenario(scenario), testing::ExitedWithCode(EXIT_SUCCESS), "");
}
