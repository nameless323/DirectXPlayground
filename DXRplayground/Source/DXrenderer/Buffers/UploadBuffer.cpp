#include "UploadBuffer.h"

namespace DirectxPlayground
{

UploadBuffer::UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount)
    : m_isConstantBuffer(isConstantBuffer)
{
    m_rawDataSize = static_cast<size_t>(elementSize);
    m_frameDataSize = m_isConstantBuffer ? GetConstantBufferByteSize(elementSize) : m_rawDataSize;
    m_bufferSize = framesCount * m_frameDataSize;

    CD3DX12_HEAP_PROPERTIES hProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC rDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);
    HRESULT hr = device.CreateCommittedResource(
        &hProps,
        D3D12_HEAP_FLAG_NONE,
        &rDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    );
    if (!SUCCEEDED(hr))
        return;

    CD3DX12_RANGE readRange(0, 0);
    m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_data));
}

UploadBuffer::~UploadBuffer()
{
    if (m_resource != nullptr)
        m_resource->Unmap(0, nullptr);
    m_data = nullptr;
}

ID3D12Resource* UploadBuffer::GetResource() const
{
    return m_resource.Get();
}

void UploadBuffer::UploadData(UINT frameIndex, const byte* data)
{
    memcpy(m_data + frameIndex * m_frameDataSize, data, m_rawDataSize);
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetFrameDataGpuAddress(UINT frame) const
{
    return m_resource->GetGPUVirtualAddress() + frame * m_frameDataSize;
}

constexpr UINT UploadBuffer::GetConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

}