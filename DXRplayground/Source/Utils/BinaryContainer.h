#pragma once

#include <cassert>
#include <functional>
#include <wrl.h>
#include <DirectXMath.h>

namespace DirectxPlayground
{
/*
 * Example of stream operators
 * struct MyStruct
* {
* public:
*     int a = 0, b = 0;
*     friend BinaryContainer& operator<<(BinaryContainer& op, const MyStruct& s)
*     {
*         op << s.a << s.b;
*         return op;
*     }
*     friend BinaryContainer& operator>>(BinaryContainer& op, MyStruct& s)
*     {
*         op >> s.a >> s.b;
*         return op;
*     }
* };
 */

class BinaryContainer
{
public:
    enum class Mode
    {
        READ,
        WRITE,
        CLOSED
    };

    static size_t DoubleSize(size_t prevSize)
    {
        return prevSize * 2;
    }

    BinaryContainer(size_t initialCapacity = 1024, std::function<size_t(size_t)> resizePolicy = [](size_t prevSize)->size_t { return BinaryContainer::DoubleSize(prevSize); });
    BinaryContainer(char* data, size_t size);
    ~BinaryContainer();

    void Close()
    {
        if (mMode == Mode::WRITE)
        {
            size_t byteSize = mCurrentPtrLocation - sizeof(size_t); // [a_vorontcov] Skip first 8 bytes for the size itself. It will be read separately.
            memcpy(mData, &byteSize, sizeof(size_t));
        }
        mMode = Mode::CLOSED;
    }

    size_t GetCapacity() const
    {
        return mCapacity;
    }

    size_t GetLastPointerOffset() const
    {
        return mCurrentPtrLocation;
    }

    Mode GetStreamMode() const
    {
        return mMode;
    }

    const char* const GetData() const
    {
        assert(mMode == Mode::CLOSED);
        return reinterpret_cast<const char*>(mData);
    }

    void Write(size_t offset, char* data, size_t size) const
    {
        memcpy(mData + offset, data, size);
    }

private:
    size_t mCapacity;
    size_t mCurrentPtrLocation;
    std::function<size_t(size_t)> mResizePolicy;
    unsigned char* mData;
    Mode mMode;
    const Mode mInitialMode;

    void Resize();

public:
    BinaryContainer& operator<< (int val);
    BinaryContainer& operator<< (float val);
    BinaryContainer& operator<< (UINT val);
    BinaryContainer& operator<< (size_t val);
    BinaryContainer& operator<< (const std::vector<int>& val);
    BinaryContainer& operator<< (const std::vector<byte>& val);
    BinaryContainer& operator<< (const std::string& val);
    BinaryContainer& operator<< (const std::pair<const unsigned char*, size_t>& val);
    template<typename T, typename A, typename = std::enable_if_t<std::is_pod_v<T>>>
    BinaryContainer& operator<< (const std::vector<T, A>& val);

    BinaryContainer& operator<< (const DirectX::XMFLOAT2& val);
    BinaryContainer& operator<< (const DirectX::XMFLOAT3& val);
    BinaryContainer& operator<< (const DirectX::XMFLOAT4& val);

    BinaryContainer& operator>> (int& val);
    BinaryContainer& operator>> (float& val);
    BinaryContainer& operator>> (UINT& val);
    BinaryContainer& operator>> (size_t& val);
    BinaryContainer& operator>> (std::vector<int>& val);
    BinaryContainer& operator>> (std::vector<byte>& val);
    BinaryContainer& operator>> (std::string& val);
    BinaryContainer& operator>> (std::pair<unsigned char*, size_t>& val);
    template<typename T, typename A, typename = std::enable_if_t<std::is_pod_v<T>>>
    BinaryContainer& operator>> (std::vector<T, A>& val);

    BinaryContainer& operator>> (DirectX::XMFLOAT2& val);
    BinaryContainer& operator>> (DirectX::XMFLOAT3& val);
    BinaryContainer& operator>> (DirectX::XMFLOAT4& val);
};

template<typename T, typename A, typename>
BinaryContainer& BinaryContainer::operator<< (const std::vector<T, A>& val)
{
    assert(mMode == Mode::WRITE);
    while (sizeof(T) * val.size() + sizeof(size_t) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(T) * val.size();
    memcpy(p, val.data(), sizeof(T) * val.size());
    return *this;
}

template<typename T, typename A, typename>
BinaryContainer& BinaryContainer::operator>> (std::vector<T, A>& val)
{
    assert(mMode == Mode::READ);
    assert(val.empty());
    size_t sz = 0;
    this->operator>>(sz);
    val.resize(sz);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sz * sizeof(T);
    memcpy(val.data(), p, sz * sizeof(T));
    return *this;
}
}
