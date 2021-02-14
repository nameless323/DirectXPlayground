#pragma once

#include <d3d12.h>
#include <wrl.h>
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount);
    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer& operator=(UploadBuffer&&) = delete;
    ~UploadBuffer();

    ID3D12Resource* GetResource() const;
    void UploadData(UINT frameIndex, const byte* data);
    template <typename T>
    void UploadData(UINT frameIndex, T& data);

    D3D12_GPU_VIRTUAL_ADDRESS GetFrameDataGpuAddress(UINT frame) const;

private:
    bool m_isConstantBuffer = false;
    size_t m_frameDataSize = 0;
    size_t m_rawDataSize = 0;
    size_t m_bufferSize = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    byte* m_data = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;

    static constexpr UINT GetConstantBufferByteSize(UINT byteSize);
};

template <typename T>
void UploadBuffer::UploadData(UINT frameIndex, T& data)
{
    UploadData(frameIndex, reinterpret_cast<byte*>(&data));
}
}
