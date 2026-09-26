//
// Created by Amo on 2026/9/22.
//

#ifndef A2_WINDOW_CLEAR_APP_H
#define A2_WINDOW_CLEAR_APP_H
#include "../utilities.h"


namespace A2WindowClear {
    static constexpr LPCWSTR CLASSNAME = L"A2WindowClear";
}

class App {
public:

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void InitializeWindow();

    void CleanUp() noexcept;

    [[nodiscard]] bool IsRunning() const {
        return bRunning_;
    }

    [[nodiscard]] HWND GetHWnd() const {
        return hWnd_;
    }

public:

    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

protected:
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //===========================
    // Initialize

    void LoadPipeline();

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
};


#endif //A2_WINDOW_CLEAR_APP_H
