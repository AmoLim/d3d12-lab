//
// Created by Amo on 2026/9/22.
//

#ifndef A2_WINDOW_CLEAR_APP_H
#define A2_WINDOW_CLEAR_APP_H

#include <memory>
#include <wrl/client.h>

#include "Dx12/Dx12Renderer.h"

namespace A2WindowClear {
    static constexpr LPCWSTR CLASSNAME = L"A2WindowClear";

    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;
}

class App {
public:

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void InitializeWindow();
    void InitializeRenderer();

    void CleanUp() noexcept;

    [[nodiscard]] bool IsRunning() const {
        return bRunning_;
    }

    [[nodiscard]] HWND GetHWnd() const {
        return hWnd_;
    }

public:


protected:
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //===========================
    // Initialize


private:
    //===========================
    // App Win32 Variables
    bool bRunning_ = true;
    bool bMinimized_ = false;
    bool bClassRegistered_ = false;

    // HINSTANCE CACHE
    HINSTANCE hInstance_ = nullptr;

    HWND hWnd_ = nullptr;

    //============================
    // Other
    int selected_ = 1;

    //============================
    // Direct12 Renderer
    std::unique_ptr<Dx12Renderer> dx12Renderer_;
};


#endif //A2_WINDOW_CLEAR_APP_H
