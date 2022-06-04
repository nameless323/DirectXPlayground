#pragma once

#include <d3d12.h>
#include <cassert>
#include <wrl.h>
#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/ResourceDX.h"
#include "External/Dx12Helpers/d3dx12.h"

namespace DirectxPlayground
{
class UploadBuffer // todo: split to upload, cb and rt
{
public:
    UploadBuffer(ID3D12Device& device, UINT elementSize, bool isConstantBuffer, UINT framesCount, bool isRtShaderRecordBuffer = false);
    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer(UploadBuffer&&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;
    UploadBuffer& operator=(UploadBuffer&&) = delete;
    ~UploadBuffer();

    ID3D12Resource* GetResource() const;
    void UploadData(UINT frameIndex, const byte* data);
    template <typename T>
    void UploadData(UINT frameIndex, const T& data);

    D3D12_GPU_VIRTUAL_ADDRESS GetFrameDataGpuAddress(UINT frame) const;
    void SetName(const std::wstring& name);

    size_t GetFrameDataSize() const;
    size_t GetBufferSize() const;

    void Unmap()
    {
        if (mResource.Get() != nullptr)
            mResource.Get()->Unmap(0, nullptr);
    }

private:
    bool mIsConstantBuffer = false;
    size_t mFrameDataSize = 0;
    size_t mRawDataSize = 0;
    size_t mBufferSize = 0;
    UINT mFramesCount = 0;
    ResourceDX mResource{ D3D12_RESOURCE_STATE_GENERIC_READ };
    byte* mData = nullptr;

    static constexpr UINT GetConstantBufferByteSize(UINT byteSize);
};

template <typename T>
void UploadBuffer::UploadData(UINT frameIndex, const T& data)
{
    assert(frameIndex < mFramesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    UploadData(frameIndex, reinterpret_cast<const byte*>(&data));
}

inline void UploadBuffer::SetName(const std::wstring& name)
{
    SetDXobjectName(mResource.Get(), name.c_str());
}

inline size_t UploadBuffer::GetFrameDataSize() const
{
    return mFrameDataSize;
}

inline size_t UploadBuffer::GetBufferSize() const
{
    return mBufferSize;
}

//////////////////////////////////////////////////////////////////////////
/// UnorderedAccessBuffer
//////////////////////////////////////////////////////////////////////////

class UnorderedAccessBuffer
{
public:
    UnorderedAccessBuffer(ID3D12GraphicsCommandList* commandList, ID3D12Device& device, UINT dataSize, const byte* initialData = nullptr, bool isStagingBuffer = false, bool isRtAccelerationStruct = false);
    UnorderedAccessBuffer(const UploadBuffer&) = delete;
    UnorderedAccessBuffer(UploadBuffer&&) = delete;
    UnorderedAccessBuffer& operator=(const UploadBuffer&) = delete;
    UnorderedAccessBuffer& operator=(UploadBuffer&&) = delete;
    ~UnorderedAccessBuffer();

    ID3D12Resource* GetResource() const;
    void UploadData(const byte* data);
    template <typename T>
    void UploadData(T& data);

    void SetName(const std::wstring& name);

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const;

private:
    UINT mBufferSize = 0;
    byte* mData = nullptr;

    ResourceDX mBuffer;
    ResourceDX mUploadBuffer{ D3D12_RESOURCE_STATE_GENERIC_READ };
    byte* mMappedData = nullptr;

    bool mIsStaging = false;
};

template <typename T>
void UnorderedAccessBuffer::UploadData(T& data)
{
    assert(sizof(T) == mBufferSize);
    UploadData(reinterpret_cast<byte*>(&data));
}

inline ID3D12Resource* UnorderedAccessBuffer::GetResource() const
{
    return mBuffer.Get();
}

inline void UnorderedAccessBuffer::UploadData(const byte* data)
{
    assert(mIsStaging && "The buffer isn't staging.");
    memcpy(mMappedData, data, mBufferSize);
}

inline D3D12_GPU_VIRTUAL_ADDRESS UnorderedAccessBuffer::GetGpuAddress() const
{
    return mBuffer.Get()->GetGPUVirtualAddress();
}

inline void UnorderedAccessBuffer::SetName(const std::wstring& name)
{
    mBuffer.SetName(name.c_str());
}
}
