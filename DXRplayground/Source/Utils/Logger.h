#pragma once

#include "External/IMGUI/imgui.h"
#include "Utils/Helpers.h"

#include <string>
#include <sstream>
#include <locale>
// See from imgui_demo.cpp

namespace DirectxPlayground
{

class ImguiLogger
{
public:
    static ImguiLogger Logger;

    ImguiLogger();

    void Clear();
    void AddLog(const std::string& msg);
    void Draw(const char* title, bool* p_open = NULL);

private:
    void AddLogInternal(const char* fmt, ...);

    ImGuiTextBuffer m_textBuffer;
    ImVector<int> m_lineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;
};

inline void ImguiLogger::Clear()
{
    m_textBuffer.clear();
    m_lineOffsets.clear();
    m_lineOffsets.push_back(0);
}

inline void ImguiLogger::AddLog(const std::string& msg)
{
    AddLogInternal("%s \n", msg.c_str());
}


namespace Internal
{
template<typename TF>
inline void WriteLog(std::stringstream& ss, const TF& f)
{
    ss << f << std::endl;
    OutputDebugStringA(ss.str().c_str());
    printf("%s\n", ss.str().c_str());
    ImguiLogger::Logger.AddLog(ss.str());
}

template<typename TF, typename ... TR>
inline void WriteLog(std::stringstream& ss, const TF& f, const TR& ... rest)
{
    ss << f;
    WriteLog(ss, rest ...);
}

template<typename ... TR>
inline void WriteLog(const char* file, int line, const TR& ... rest)
{
    std::stringstream ss;
    ss << file << "(" << line << "): " << " | ";
    WriteLog(ss, rest...);
}

template<typename TF>
inline void WriteLogW(std::wstringstream& ss, const TF& f)
{
    ss << f << std::endl;
    OutputDebugStringW(ss.str().c_str());
    wprintf(L"%ls\n", ss.str().c_str());
    ImguiLogger::Logger.AddLog(WstrToStr(ss.str()));
}

template<typename TF, typename ... TR>
inline void WriteLogW(std::wstringstream& ss, const TF& f, const TR& ... rest)
{
    ss << f;
    WriteLogW(ss, rest ...);
}

template<typename ... TR>
inline void WriteLogW(const char* file, int line, const TR& ... rest)
{
    std::wstringstream ss;
    ss << file << "(" << line << "): " << " | ";
    WriteLogW(ss, rest...);
}
}

#ifdef _DEBUG
#define LOG(...) DirectxPlayground::Internal::WriteLog(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_W(...) DirectxPlayground::Internal::WriteLogW(__FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG(...)
#define LOG_W(...)
#endif // _DEBUG

}
