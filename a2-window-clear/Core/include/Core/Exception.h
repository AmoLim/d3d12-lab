//
// Created by Amo on 2026/9/28.
//

#ifndef A2_WINDOW_CLEAR_EXCEPTION_H
#define A2_WINDOW_CLEAR_EXCEPTION_H

#include "Core/DxException.h"
#include "Core/Win32Exception.h"

#define ThrowIfFailed(x)               \
{                                      \
    HRESULT hr__ = (x);                \
    if (FAILED(hr__))                  \
    {                                  \
        throw DxException(             \
            hr__,                      \
            #x,                        \
            __FILE__,                  \
            __LINE__);                 \
    }                                  \
}

#define ThrowIfFailedWin32(x, __EXP__) \
{                                      \
    BOOL r__ = (x);                    \
    if (!r__)                          \
    {                                  \
        throw Win32Exception(          \
            GetLastError(),            \
            __EXP__,                   \
            __FILE__,                  \
            __LINE__);                 \
    }                                  \
}

#endif //A2_WINDOW_CLEAR_EXCEPTION_H
