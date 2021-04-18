#pragma once

#include "External/IMGUI/imgui.h"

#include <string>
#include <sstream>
#include <locale>
#include <codecvt>
#include <windows.h>
// Taken from imgui_demo.cpp

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

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ImguiLogger
{
    static ImguiLogger Logger;

    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool                AutoScroll;
    bool                ScrollToBottom;

    ImguiLogger()
    {
        AutoScroll = true;
        ScrollToBottom = false;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void AddLog(const std::string& msg)
    {
        AddLogInternal("%s \n", msg.c_str());
    }

    void    AddLogInternal(const char* fmt, ...)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
        if (AutoScroll)
            ScrollToBottom = true;
    }

    void    Draw(const char* title, bool* p_open = NULL)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            if (ImGui::Checkbox("Auto-scroll", &AutoScroll))
                if (AutoScroll)
                    ScrollToBottom = true;
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
            // especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char* line_start = buf + LineOffsets[line_no];
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
            // Here we instead demonstrate using the clipper to only process lines that are within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
            // Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
            // Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (ScrollToBottom)
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::End();
    }
};


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

#define LOG(...) DirectxPlayground::Internal::WriteLog(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_W(...) DirectxPlayground::Internal::WriteLogW(__FILE__, __LINE__, __VA_ARGS__)
}
