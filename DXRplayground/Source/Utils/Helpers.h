#pragma once

#include <string>
#include <windows.h>
#include <codecvt>

namespace DirectxPlayground
{
inline std::string WstrToStr(std::wstring s)
{
    // [a_vorontcov] Windows specific.
    if (s.empty())
        return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &s[0], (int)s.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

template <typename T>
inline void SafeDelete(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}

inline UINT Log2(UINT v)
{
    return static_cast<UINT>(log2(8));
}

inline UINT NextPOT(UINT v)
{
    return static_cast<UINT>(pow(2, Log2(v)));
}
}
