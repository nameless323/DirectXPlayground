#include "UploadBuffer.h"

#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{

UploadBuffer::UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount, bool isRtShaderRecordBuffer /* = false */)
    : m_isConstantBuffer(isConstantBuffer)
    , m_framesCount(framesCount)
{
    assert((isConstantBuffer && isRtShaderRecordBuffer) != 1 && "Buffer cant be both constant and shader record buffer");

    m_rawDataSize = static_cast<size_t>(elementSize);
    m_frameDataSize = m_isConstantBuffer ? GetConstantBufferByteSize(elementSize) : m_rawDataSize;
    //if (isRtShaderRecordBuffer)
    //    m_frameDataSize = Align(elementSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    m_bufferSize = framesCount * m_frameDataSize;

    CD3DX12_HEAP_PROPERTIES hProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC rDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);
    HRESULT hr = device.CreateCommittedResource(
        &hProps,
        D3D12_HEAP_FLAG_NONE,
        &rDesc,
        m_resource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(m_resource.GetAddressOf())
    );
    if (!SUCCEEDED(hr))
        return;

    CD3DX12_RANGE readRange(0, 0);
    m_resource.Get()->Map(0, &readRange, reinterpret_cast<void**>(&m_data));
}

UploadBuffer::~UploadBuffer()
{
    if (m_resource.Get() != nullptr)
        m_resource.Get()->Unmap(0, nullptr);
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
    return m_resource.Get()->GetGPUVirtualAddress() + frame * m_frameDataSize;
}

constexpr UINT UploadBuffer::GetConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

//////////////////////////////////////////////////////////////////////////
/// UnorderedAccessBuffer
//////////////////////////////////////////////////////////////////////////

UnorderedAccessBuffer::UnorderedAccessBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Device& device, UINT dataSize, const byte* initialData /*= nullptr*/, bool isStagingBuffer /*= false*/, bool isRtAccelerationStruct /*= false*/) // todo: just pass the init state
    : m_bufferSize(dataSize)
    , m_isStaging(isStagingBuffer)
{
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES state = initialData == nullptr ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST;
    if (isRtAccelerationStruct)
        state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    m_buffer.SetInitialState(state);
    ThrowIfFailed(device.CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        m_buffer.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(m_buffer.GetAddressOf())));

    if (initialData != nullptr)
    {
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device.CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            m_uploadBuffer.GetCurrentState(),
            nullptr,
            IID_PPV_ARGS(m_uploadBuffer.GetAddressOf())));

        UINT8* mappedBuffer = nullptr;
        CD3DX12_RANGE range(0, 0);
        m_uploadBuffer.Get()->Map(0, &range, reinterpret_cast<void**>(&mappedBuffer));
        memcpy(mappedBuffer, initialData, dataSize);
        m_uploadBuffer.Get()->Unmap(0, nullptr);
        commandList->CopyResource(m_buffer.Get(), m_uploadBuffer.Get());
        m_buffer.Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    if (isStagingBuffer)
    {
        CD3DX12_RANGE readRange(0, 0);
        m_buffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&m_data));
    }
}

UnorderedAccessBuffer::~UnorderedAccessBuffer()
{
    if (m_isStaging)
    {
        if (m_buffer.Get() != nullptr)
            m_buffer.Get()->Unmap(0, nullptr);
        m_data = nullptr;
    }
}

}