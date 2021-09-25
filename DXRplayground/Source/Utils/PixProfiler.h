#pragma once

#include <string>
#include <d3d12.h>

#include "pix3.h"

namespace DirectxPlayground::PixProfiler
{
void InitGpuProfiler(HWND hwnd)
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

ScopedGpuEvent::ScopedGpuEvent(ID3D12GraphicsCommandList* commandList, const std::string& name) : m_cmdList(commandList)
{
    PIXBeginEvent(m_cmdList, PIX_COLOR(0, 0, 254), name.c_str());
}

ScopedGpuEvent::~ScopedGpuEvent()
{
    PIXEndEvent(m_cmdList);
}

void BeginCaptureGpuFrame()
{
    PIXCaptureParameters params{};
    PIXBeginCapture(PIX_CAPTURE_GPU, &params);
}

void EndCaptureGpuFrame()
{
    PIXEndCapture(false);
}

void CaptureNextFrame()
{
    PIXGpuCaptureNextFrames(L"", 1);
}

}
