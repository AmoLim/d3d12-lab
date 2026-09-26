//
// Created by Amo on 2026/9/21.
//

#ifndef A2_WIN32_PRACTICE_APP_H
#define A2_WIN32_PRACTICE_APP_H

#include <windows.h>
#include "utilities.h"

class App {
public:
    void InitializeWindow();
    void Shutdown();

    [[nodiscard]] HWND GetWindowHandle() const {
        return hwnd_;
    }

    [[nodiscard]] bool IsRunning() const {
        return bRunning_;
    }

    [[nodiscard]] bool IsMinimized() const {
        return bMinimized_;
    }

    [[nodiscard]] int GetSelected() const {
        return selected_;
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static constexpr UINT width = 1280;
    static constexpr UINT height = 720;

    HWND hwnd_ = nullptr;
    bool bRunning_ = true;
    bool bMinimized_ = false;
    int selected_ = 1;

    bool bClassRegistered_ = false;
};


#endif //A2_WIN32_PRACTICE_APP_H
