#pragma once

#include <string>
#include <d3d12.h>

#include "pix3.h"

namespace DirectxPlayground::PixProfiler
{
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
    PIXCaptureParameters params{};
    PIXBeginCapture(PIX_CAPTURE_GPU, &params);
}

inline void EndCaptureGpuFrame()
{
    PIXEndCapture(false);
}

inline void CaptureNextFrame()
{
    PIXGpuCaptureNextFrames(L"", 1);
}
}

#define GPU_SCOPED_EVENT(ctx, name) DirectxPlayground::PixProfiler::ScopedGpuEvent pix__scoped_profile___(context.CommandList, name)
