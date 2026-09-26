//
// Created by Amo on 2026/9/21.
//

#ifndef A2_WINDOW_CLEAR_UTILITIES_H
#define A2_WINDOW_CLEAR_UTILITIES_H

#include <stdexcept>
#include <string>
#include <windows.h>

namespace Utils {
    inline void CheckWin32(BOOL success, const char* operation) {
        if (!success) {
            const DWORD error = GetLastError();
            throw std::runtime_error(
                std::string(operation) + ": Win32 " + std::to_string(error)
                );
        }
    }
}

#endif //A2_WINDOW_CLEAR_UTILITIES_H
