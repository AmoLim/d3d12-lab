//
// Created by Amo on 2026/9/28.
//

#ifndef A2_WINDOW_CLEAR_DXEXCEPTION_H
#define A2_WINDOW_CLEAR_DXEXCEPTION_H
#include <exception>
#include <cstdint>
#include <dxgi.h>
#include <string>

class DxException : public std::exception {
public:
    explicit DxException(HRESULT hr, const char* expression, const char* file, int line);

    [[nodiscard]] HRESULT ErrorCode() const noexcept;

    [[nodiscard]] const char* what() const noexcept override;

private:

    static std::string Hex(HRESULT hr);

private:
    HRESULT mHResult = {};
    uint32_t mLine = 0;

    std::string mExpression;
    std::string mFile;
    std::string mMessage;


};


#endif //A2_WINDOW_CLEAR_DXEXCEPTION_H
