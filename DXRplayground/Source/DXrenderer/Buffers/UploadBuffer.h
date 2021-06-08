#pragma once

#include <d3d12.h>
#include <cassert>
#include <wrl.h>
#include "DXrenderer/DXhelpers.h"
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
    void UploadData(UINT frameIndex, T& data);

    D3D12_GPU_VIRTUAL_ADDRESS GetFrameDataGpuAddress(UINT frame) const;
    void SetName(const std::wstring& name);

    size_t GetFrameDataSize() const;
    size_t GetBufferSize() const;

    void Unmap()
    {
        if (m_resource != nullptr)
            m_resource->Unmap(0, nullptr);
    }

private:
    bool m_isConstantBuffer = false;
    size_t m_frameDataSize = 0;
    size_t m_rawDataSize = 0;
    size_t m_bufferSize = 0;
    UINT m_framesCount = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    byte* m_data = nullptr;

    static constexpr UINT GetConstantBufferByteSize(UINT byteSize);
};

template <typename T>
void UploadBuffer::UploadData(UINT frameIndex, T& data)
{
    assert(frameIndex < m_framesCount && "Asked frame index for the buffer is bigger than maxFrames for this buffer");
    UploadData(frameIndex, reinterpret_cast<byte*>(&data));
}

inline void UploadBuffer::SetName(const std::wstring& name)
{
    SetDXobjectName(m_resource.Get(), name.c_str());
}

inline size_t UploadBuffer::GetFrameDataSize() const
{
    return m_frameDataSize;
}

inline size_t UploadBuffer::GetBufferSize() const
{
    return m_bufferSize;
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
    UINT m_bufferSize = 0;
    byte* m_data = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
    byte* m_mappedData = nullptr;

    bool m_isStaging = false;
};

template <typename T>
void UnorderedAccessBuffer::UploadData(T& data)
{
    assert(sizof(T) == m_bufferSize);
    UploadData(reinterpret_cast<byte*>(&data));
}

inline ID3D12Resource* UnorderedAccessBuffer::GetResource() const
{
    return m_buffer.Get();
}

inline void UnorderedAccessBuffer::UploadData(const byte* data)
{
    assert(m_isStaging && "The buffer isn't staging.");
    memcpy(m_mappedData, data, m_bufferSize);
}

inline D3D12_GPU_VIRTUAL_ADDRESS UnorderedAccessBuffer::GetGpuAddress() const
{
    return m_buffer->GetGPUVirtualAddress();
}

inline void UnorderedAccessBuffer::SetName(const std::wstring& name)
{
    SetDXobjectName(m_buffer.Get(), name.c_str());
}
}
