#pragma once

#ifdef _DEBUG
#include <sstream>

#define LOG_(msg, file, line) \
{ \
    std::stringstream ss___; \
    ss___ << file << "(" << line << "): " << msg << std::endl; \
    OutputDebugStringA(ss___.str().c_str()); \
    printf("%s\n", ss___.str().c_str()); \
}
#define LOG_W_(msg, file, line) \
{ \
    std::wstringstream ss___; \
    ss___ << file << "(" << line << "): " << msg << std::endl; \
    OutputDebugStringW(ss___.str().c_str()); \
    wprintf(L"%ls\n", ss___.str().c_str()); \
}
#define LOG(msg) LOG_(msg, __FILE__, __LINE__)
#define LOG_W(msg) LOG_W_(msg, __FILE__, __LINE__)
#else
#define LOG(msg)
#define LOG_W(msg)
#endif