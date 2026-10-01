//
// Created by Amo on 2026/9/30.
//

#ifndef A2_WINDOW_CLEAR_WIN32EXCEPTION_H
#define A2_WINDOW_CLEAR_WIN32EXCEPTION_H

#include <string>
#include <exception>
#include <cstdint>
#include <windows.h>


class Win32Exception : public std::exception {
public:
    explicit Win32Exception(DWORD errorCode, const char* expression, const char *file, const int line);

    [[nodiscard]] DWORD ErrorCode() const;

    [[nodiscard]] char const *what() const override;

private:
    DWORD mErrorCode = {};
    uint32_t mLine = 0;

    std::string mExpression;
    std::string mFile;
    std::string mMessage;
};


#endif //A2_WINDOW_CLEAR_WIN32EXCEPTION_H
