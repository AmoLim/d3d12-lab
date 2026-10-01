//
// Created by Amo on 2026/9/30.
//

#include "Core/Win32Exception.h"

Win32Exception::Win32Exception(const DWORD errorCode, const char *expression, const char *file, const int line)
    : mErrorCode(errorCode), mLine(line), mExpression(expression), mFile(file) {
    mMessage = mExpression + ": " + mFile + ":" + std::to_string(mLine);
}

DWORD Win32Exception::ErrorCode() const {
    return mErrorCode;
}

char const * Win32Exception::what() const {
    return mMessage.c_str();
}
