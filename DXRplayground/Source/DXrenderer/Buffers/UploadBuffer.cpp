#include "UploadBuffer.h"

#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/DXhelpers.h"

namespace DirectxPlayground
{

UploadBuffer::UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount, bool isRtShaderRecordBuffer /* = false */)
    : mIsConstantBuffer(isConstantBuffer)
    , mFramesCount(framesCount)
{
    assert((isConstantBuffer && isRtShaderRecordBuffer) != 1 && "Buffer cant be both constant and shader record buffer");

    mRawDataSize = static_cast<size_t>(elementSize);
    mFrameDataSize = mIsConstantBuffer ? GetConstantBufferByteSize(elementSize) : mRawDataSize;
    //if (isRtShaderRecordBuffer)
    //    m_frameDataSize = Align(elementSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    mBufferSize = framesCount * mFrameDataSize;

    CD3DX12_HEAP_PROPERTIES hProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC rDesc = CD3DX12_RESOURCE_DESC::Buffer(mBufferSize);
    HRESULT hr = device.CreateCommittedResource(
        &hProps,
        D3D12_HEAP_FLAG_NONE,
        &rDesc,
        mResource.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(mResource.GetAddressOf())
    );
    if (!SUCCEEDED(hr))
        return;

    CD3DX12_RANGE readRange(0, 0);
    mResource.Get()->Map(0, &readRange, reinterpret_cast<void**>(&mData));
}

UploadBuffer::~UploadBuffer()
{
    if (mResource.Get() != nullptr)
        mResource.Get()->Unmap(0, nullptr);
    mData = nullptr;
}

ID3D12Resource* UploadBuffer::GetResource() const
{
    return mResource.Get();
}

void UploadBuffer::UploadData(UINT frameIndex, const byte* data)
{
    assert(frameIndex < mFramesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    memcpy(mData + frameIndex * mFrameDataSize, data, mRawDataSize);
}

D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer::GetFrameDataGpuAddress(UINT frame) const
{
    assert(frame < mFramesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    return mResource.Get()->GetGPUVirtualAddress() + frame * mFrameDataSize;
}

constexpr UINT UploadBuffer::GetConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

//////////////////////////////////////////////////////////////////////////
/// UnorderedAccessBuffer
//////////////////////////////////////////////////////////////////////////

UnorderedAccessBuffer::UnorderedAccessBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Device& device, UINT dataSize, const byte* initialData /*= nullptr*/, bool isStagingBuffer /*= false*/, bool isRtAccelerationStruct /*= false*/) // todo: just pass the init state
    : mBufferSize(dataSize)
    , mIsStaging(isStagingBuffer)
{
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES state = initialData == nullptr ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST;
    if (isRtAccelerationStruct)
        state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    mBuffer.SetInitialState(state);
    ThrowIfFailed(device.CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        mBuffer.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(mBuffer.GetAddressOf())));

    if (initialData != nullptr)
    {
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device.CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            mUploadBuffer.GetCurrentState(),
            nullptr,
            IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

        UINT8* mappedBuffer = nullptr;
        CD3DX12_RANGE range(0, 0);
        mUploadBuffer.Get()->Map(0, &range, reinterpret_cast<void**>(&mappedBuffer));
        memcpy(mappedBuffer, initialData, dataSize);
        mUploadBuffer.Get()->Unmap(0, nullptr);
        commandList->CopyResource(mBuffer.Get(), mUploadBuffer.Get());
        mBuffer.Transition(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    if (isStagingBuffer)
    {
        CD3DX12_RANGE readRange(0, 0);
        mBuffer.Get()->Map(0, &readRange, reinterpret_cast<void**>(&mData));
    }
}

UnorderedAccessBuffer::~UnorderedAccessBuffer()
{
    if (mIsStaging)
    {
        if (mBuffer.Get() != nullptr)
            mBuffer.Get()->Unmap(0, nullptr);
        mData = nullptr;
    }
}

}