#include "UploadBuffer.h"

#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{

UploadBuffer::UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount)
    : m_isConstantBuffer(isConstantBuffer)
    , m_framesCount(framesCount)
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
    assert(frameIndex < m_framesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    memcpy(m_data + frameIndex * m_frameDataSize, data, m_rawDataSize);
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetFrameDataGpuAddress(UINT frame) const
{
    assert(frame < m_framesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    return m_resource->GetGPUVirtualAddress() + frame * m_frameDataSize;
}

constexpr UINT UploadBuffer::GetConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

//////////////////////////////////////////////////////////////////////////
/// UnorderedAccessBuffer
//////////////////////////////////////////////////////////////////////////

UnorderedAccessBuffer::UnorderedAccessBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Device& device, UINT dataSize, const byte* initialData /*= nullptr*/, bool isStagingBuffer /*= false*/)
    : m_bufferSize(dataSize)
    , m_isStaging(isStagingBuffer)
{
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES state = initialData == nullptr ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST;
    ThrowIfFailed(device.CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        state,
        nullptr,
        IID_PPV_ARGS(&m_buffer)));

    if (initialData != nullptr)
    {
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device.CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_uploadBuffer)));

        UINT8* mappedBuffer = nullptr;
        CD3DX12_RANGE range(0, 0);
        m_uploadBuffer->Map(0, &range, reinterpret_cast<void**>(&mappedBuffer));
        memcpy(mappedBuffer, initialData, dataSize);
        m_uploadBuffer->Unmap(0, nullptr);
        commandList->CopyResource(m_buffer.Get(), m_uploadBuffer.Get());
        CD3DX12_RESOURCE_BARRIER toDest = CD3DX12_RESOURCE_BARRIER::Transition(m_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &toDest);
    }

    if (isStagingBuffer)
    {
        CD3DX12_RANGE readRange(0, 0);
        m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_data));
    }
}

UnorderedAccessBuffer::~UnorderedAccessBuffer()
{
    if (m_isStaging)
    {
        if (m_buffer != nullptr)
            m_buffer->Unmap(0, nullptr);
        m_data = nullptr;
    }
}

}