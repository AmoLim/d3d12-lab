//
// Created by Amo on 2026/9/28.
//

#include "Core/DxException.h"

#include <iomanip>
#include <sstream>

DxException::DxException(HRESULT hr, const char *expression, const char *file, const int line)
    : mHResult(hr), mLine(line), mExpression(expression), mFile(file) {
    mMessage = mExpression + ": " + mFile + ":" + std::to_string(mLine) + "\nError code: " + Hex(mHResult);
}

HRESULT DxException::ErrorCode() const noexcept {
    return mHResult;
}

const char * DxException::what() const noexcept {
    return mMessage.c_str();
}

std::string DxException::Hex(HRESULT hr) {
    std::ostringstream text;
    text << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(hr) <<
            std::endl;
    return text.str();
}
