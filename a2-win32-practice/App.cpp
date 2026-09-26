//
// Created by Amo on 2026/9/21.
//

#include "wrl/client.h"

#include "App.h"

#include <iostream>

namespace a2_win32_practice {
    constexpr auto className = L"A2WindowsPractice";
}

void App::InitializeWindow() {
    // 拿取系统资源句柄
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    CheckWin32(instance != nullptr, "GetModuleHandleW");

    // 定义WindowClass参数
    WNDCLASSW wndClass{};
    wndClass.hInstance = instance;
    wndClass.lpfnWndProc = WndProc;
    wndClass.lpszClassName =a2_win32_practice::className;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    CheckWin32(wndClass.hCursor != nullptr, "LoadCursorW");
    // 注册WindowClass
    CheckWin32(RegisterClassW(&wndClass), "RegisterClassW");
    bClassRegistered_ = true;

    // 检查Style与window窗口RECT的适配
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT outer{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    CheckWin32(AdjustWindowRect(&outer, style, FALSE), "AdjustWindowRect");

    // 创建窗口并获得窗口句柄
    hwnd_ = CreateWindowExW(
        0,
        a2_win32_practice::className,
        L"A2 Win32 | Key : 1",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        outer.right - outer.left,
        outer.bottom - outer.top,
        nullptr,
        nullptr,
        instance,
        this
        );

    CheckWin32(hwnd_ != nullptr, "CreateWindowExW");
}

void App::Shutdown(){
    if (hwnd_ && IsWindow(hwnd_)) {
        CheckWin32(DestroyWindow(hwnd_), "DestroyWindow");
    }

    if (bClassRegistered_) {
        CheckWin32(UnregisterClassW(a2_win32_practice::className, GetModuleHandleW(nullptr)), "UnregisterClassW");
        bClassRegistered_ = false;
    }
}

LRESULT App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;

    if (msg == WM_NCCREATE) {
        // 获得的msg为正在创建非客户区域，尝试将App*绑定到GWLP_USERDATA中
        const auto* creation = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(creation->lpCreateParams);
        if (!app) return FALSE;

        // A zero previous value can also mean success.
        /**
         * 因为这里SetWindowLongPtrW函数返回的是旧值，这个旧值可能是0，不能依靠简单的previous == 0就判断其失败，
         * 这里先SetLastError 为 ERROR_SUCCESS这个零值ERROR码，再看SetWindowLongPtrW后该错误码是否被改写来判断是否真的失败。
         * **/

        SetLastError(ERROR_SUCCESS);
        const LONG_PTR previous = SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        if (previous == 0 && GetLastError() != ERROR_SUCCESS) { return FALSE; }
        return TRUE;
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!app) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return app->HandleMessage(hwnd, msg, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {

            // 取得HDC和此次paint所需的信息
            PAINTSTRUCT paint{};
            const HDC deviceContext = BeginPaint(hwnd, &paint);
            // 只关注PaintStructure.rcPaint字段，将这个这块区域给重新涂成窗口背景色
            FillRect(deviceContext, &paint.rcPaint, GetSysColorBrush(COLOR_WINDOW));
            EndPaint(hwnd, &paint);

            return 0;
        }

        case WM_SIZE : {
            bMinimized_ = (wParam == SIZE_MINIMIZED);
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam >= '1' && wParam <= '3') {
                selected_ = static_cast<int>(wParam - '0');
            }
            if (wParam == VK_ESCAPE) {
                bRunning_ = false;
            }
            return 0;
        }

        case WM_CLOSE: {
            bRunning_ = false;
            return 0;
        }

        case WM_DESTROY: {
            bRunning_ = false;
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
