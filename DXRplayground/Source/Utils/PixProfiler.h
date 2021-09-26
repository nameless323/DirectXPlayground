#pragma once

#include <string>
#include <d3d12.h>
#include <chrono>
#include <ctime>

#include "pix3.h"

namespace DirectxPlayground::PixProfiler
{
inline std::wstring GetCurrentCaptureName()
{
    std::time_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char tmBuff[30];
    ctime_s(tmBuff, sizeof(tmBuff), &timeNow);
    std::string filename = std::string("pix_capture_") + std::string(tmBuff);
    return std::wstring(filename.begin(), filename.end());
}

inline void InitGpuProfiler(HWND hwnd)
{
    PIXLoadLatestWinPixGpuCapturerLibrary();
    PIXSetTargetWindow(hwnd);
}

class ScopedGpuEvent
{
public:
    ScopedGpuEvent(ID3D12GraphicsCommandList* commandList, const std::string& name);
    ~ScopedGpuEvent();

private:
    ID3D12GraphicsCommandList* m_cmdList = nullptr;
};

inline ScopedGpuEvent::ScopedGpuEvent(ID3D12GraphicsCommandList* commandList, const std::string& name) : m_cmdList(commandList)
{
    PIXBeginEvent(m_cmdList, PIX_COLOR(0, 0, 254), name.c_str());
}

inline ScopedGpuEvent::~ScopedGpuEvent()
{
    PIXEndEvent(m_cmdList);
}

inline void BeginCaptureGpuFrame()
{
    std::wstring filename = PIX_CAPTURES_DIR_W + GetCurrentCaptureName();
    PIXCaptureParameters params{};
    params.GpuCaptureParameters.FileName = filename.c_str();
    PIXBeginCapture(PIX_CAPTURE_GPU, &params);
}

inline void EndCaptureGpuFrame()
{
    PIXEndCapture(false);
}

inline void CaptureNextFrame()
{
    PIXGpuCaptureNextFrames((PIX_CAPTURES_DIR_W + GetCurrentCaptureName()).c_str(), 1);
}
}

#define GPU_SCOPED_EVENT(ctx, name) DirectxPlayground::PixProfiler::ScopedGpuEvent pix__scoped_profile___(context.CommandList, name)
