//
// Created by Amo on 2026/9/22.
//

#include "App.h"

#include <windows.h>

LRESULT App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;

    if (msg == WM_NCCREATE) {

        const auto creation = reinterpret_cast<LPCREATESTRUCT>(lParam);
        app = static_cast<App*>(creation->lpCreateParams);
        if (!app) {
            return FALSE;
        }

        /**
         * 因为这里SetWindowLongPtrW函数返回的是旧值，这个旧值可能是0，不能依靠简单的previous == 0就判断其失败，
         * 这里先SetLastError 为 ERROR_SUCCESS这个零值ERROR码，再看SetWindowLongPtrW后该错误码是否被改写来判断是否真的失败。
         **/

        SetLastError(ERROR_SUCCESS);
        if (const LONG_PTR prev = SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            prev == 0 && GetLastError() != ERROR_SUCCESS) {
            return FALSE;
        }

        return TRUE;
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (!app) {
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return app->HandleMessage(hWnd, msg, wParam, lParam);
}

void App::InitializeWindow() {
    // 拿取当前.exe Windows资源Instance句柄
    hInstance_ = GetModuleHandle(nullptr);
    Utils::CheckWin32(hInstance_ != nullptr, "GetModuleHandleW");

    // 填入WNDCLASS参数
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = A2WindowClear::CLASSNAME;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    Utils::CheckWin32(wc.hCursor != nullptr, "LoadCursorW");

    // 注册Window Class
    Utils::CheckWin32(RegisterClassW(&wc), "RegisterClassW");
    bClassRegistered_ = true;

    // check if can Adjust Target style to rect
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT outer{0, 0, WIDTH, HEIGHT};
    Utils::CheckWin32(AdjustWindowRect(&outer, style, FALSE), "AdjustWindowRect");

    hWnd_ = CreateWindowExW(
        0, A2WindowClear::CLASSNAME,
        L"A2 Window Clear", style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIDTH, HEIGHT,
        nullptr, nullptr,
        hInstance_, this);

    Utils::CheckWin32(hWnd_ != nullptr, "CreateWindowW");
}

void App::CleanUp() noexcept {
    if (hWnd_ != nullptr) {
        if (!DestroyWindow(hWnd_)) {
            const DWORD error = GetLastError();

            // TODO: Log error
            (void) error;
            return;
        }
    }

    if (bClassRegistered_) {
        if (!UnregisterClassW(A2WindowClear::CLASSNAME, hInstance_)) {
            const DWORD error = GetLastError();

            // TODO: Log error
            (void) error;
            return;
        }
        bClassRegistered_ = false;
    }
}

LRESULT App::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hWnd, &ps);
            FillRect(dc, &ps.rcPaint, GetSysColorBrush(COLOR_WINDOW));
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_SIZE : {
            bMinimized_ = wParam == SIZE_MINIMIZED;
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                bRunning_ = false;
                return 0;
            }
            break;
        }

        case WM_NCDESTROY: {
            hWnd_ = nullptr;
            return 0;
        }

        case WM_CLOSE : {
            bRunning_ = false;
            return 0;
        }

        case WM_DESTROY: {
            bRunning_ = false;
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void App::LoadPipeline() {

}
