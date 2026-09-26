//
// Created by Amo on 2026/9/21.
//

#ifndef A2WIN32PRACTICE_UTILITIES_H
#define A2WIN32PRACTICE_UTILITIES_H

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <windows.h>

inline std::string Hex(HRESULT result) {
    std::ostringstream text;

    text << "0x" << std::hex << std::uppercase << std::setfill('0') <<
        std::setw(8) << static_cast<std::uint32_t>(result);
}

inline void Check(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        throw std::runtime_error(std::string(operation) + ": " + Hex(result));
    }
}

inline void CheckWin32(BOOL success, const char* operation) {
    if (!success) {
        const DWORD error = GetLastError();
        throw std::runtime_error(
            std::string(operation) + ": Win32 " + std::to_string(error));
    }
}

#endif //A2WIN32PRACTICE_UTILITIES_H
