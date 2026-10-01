#ifndef A2_WINDOW_CLEAR_WINDOWCONFIG_H
#define A2_WINDOW_CLEAR_WINDOWCONFIG_H

#include <windows.h>

namespace A2WindowClear {
    static constexpr LPCWSTR CLASSNAME = L"A2WindowClear";

    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;
    static constexpr FLOAT clearColor[4][4] = {
        {0.08, 0.20, 0.36, 1.0},
        {0.08, 0.20, 0.36, 1.0},
        {0.10, 0.55, 0.22, 1.0},
        {0.65, 0.12, 0.18, 1.0}
    };
}

#endif //A2_WINDOW_CLEAR_WINDOWCONFIG_H
