#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "External/Dx12Helpers/d3dx12.h"
#include "DXrenderer/DXhelpers.h"
#include "DXrenderer/ResourceDX.h"

namespace DirectxPlayground
{
class HeapBuffer
{
public:
    HeapBuffer(const byte* data, UINT dataSize, ID3D12GraphicsCommandList* commandList, ID3D12Device* device, D3D12_RESOURCE_STATES destinationState);
    HeapBuffer(const HeapBuffer&) = delete;
    HeapBuffer(HeapBuffer&&) = delete;
    HeapBuffer& operator=(const HeapBuffer&) = delete;
    HeapBuffer& operator=(HeapBuffer&&) = delete;
    ~HeapBuffer() = default;

    ID3D12Resource* GetBuffer() const;

private:
    ResourceDX m_buffer{ D3D12_RESOURCE_STATE_COPY_DEST };
    ResourceDX m_uploadBuffer{ D3D12_RESOURCE_STATE_GENERIC_READ }; // [a_vorontcov] TODO:: Check if command list was executed and release ptr. But maybe its not nessesary.
};

inline ID3D12Resource* HeapBuffer::GetBuffer() const
{
    return m_buffer.Get();
}

inline HeapBuffer::HeapBuffer(const byte* data, UINT dataSize, ID3D12GraphicsCommandList* commandList, ID3D12Device* device, D3D12_RESOURCE_STATES destinationState)
{
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        m_uploadBuffer.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(m_uploadBuffer.GetAddressOf())));

    UINT8* mappedBuffer = nullptr;
    CD3DX12_RANGE range(0, 0);
    m_uploadBuffer.Get()->Map(0, &range, reinterpret_cast<void**>(&mappedBuffer));
    memcpy(mappedBuffer, data, dataSize);
    m_uploadBuffer.Get()->Unmap(0, nullptr);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        m_buffer.GetCurrentState(),
        nullptr,
        IID_PPV_ARGS(m_buffer.GetAddressOf())));

    commandList->CopyResource(m_buffer.Get(), m_uploadBuffer.Get());
    m_buffer.Transition(commandList, destinationState);
}

//////////////////////////////////////////////////////////////////////////
/// Index Buffer
//////////////////////////////////////////////////////////////////////////

class IndexBuffer
{
public:
    IndexBuffer(const byte* indexData, UINT indexDataSize, ID3D12GraphicsCommandList* commandList, ID3D12Device* device, DXGI_FORMAT indexBufferFormat);
    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer& operator=(IndexBuffer&&) = delete;
    ~IndexBuffer();

    ID3D12Resource* GetIndexBuffer() const;
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const;

private:
    HeapBuffer* m_buffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
};

inline const D3D12_INDEX_BUFFER_VIEW& IndexBuffer::GetIndexBufferView() const
{
    return m_indexBufferView;
}

inline ID3D12Resource* IndexBuffer::GetIndexBuffer() const
{
    return m_buffer->GetBuffer();
}

inline IndexBuffer::IndexBuffer(const byte* indexData, UINT indexDataSize, ID3D12GraphicsCommandList* commandList, ID3D12Device* device, DXGI_FORMAT indexBufferFormat)
{
    m_buffer = new HeapBuffer(indexData, indexDataSize, commandList, device, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    m_indexBufferView.BufferLocation = m_buffer->GetBuffer()->GetGPUVirtualAddress();
    m_indexBufferView.Format = indexBufferFormat;
    m_indexBufferView.SizeInBytes = indexDataSize;
}

inline IndexBuffer::~IndexBuffer()
{
    delete m_buffer;
}

//////////////////////////////////////////////////////////////////////////
/// Vertex Buffer
//////////////////////////////////////////////////////////////////////////

class VertexBuffer
{
public:
    VertexBuffer(const byte* vertexData, UINT vertexDataSize, UINT vertexStride, ID3D12GraphicsCommandList* commandList, ID3D12Device* device);
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer& operator=(VertexBuffer&&) = delete;
    ~VertexBuffer();

    ID3D12Resource* GetVertexBuffer() const;
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const;

private:
    HeapBuffer* m_buffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
};

inline const D3D12_VERTEX_BUFFER_VIEW& VertexBuffer::GetVertexBufferView() const
{
    return m_vertexBufferView;
}

inline ID3D12Resource* VertexBuffer::GetVertexBuffer() const
{
    return m_buffer->GetBuffer();
}

inline VertexBuffer::VertexBuffer(const byte* vertexData, UINT vertexDataSize, UINT vertexStride, ID3D12GraphicsCommandList* commandList, ID3D12Device* device)
{
    m_buffer = new HeapBuffer(vertexData, vertexDataSize, commandList, device, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_vertexBufferView.BufferLocation = m_buffer->GetBuffer()->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = vertexStride;
    m_vertexBufferView.SizeInBytes = vertexDataSize;
}

inline VertexBuffer::~VertexBuffer()
{
    delete m_buffer;
}
}