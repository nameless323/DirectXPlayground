#include "Utils/BinaryContainer.h"

#include <cassert>

namespace DirectxPlayground
{
BinaryContainer::BinaryContainer(size_t initialCapacity, std::function<size_t(size_t)> resizePolicy)
    : mCapacity(initialCapacity)
    , mCurrentPtrLocation(sizeof(size_t)) // During .Close() in the first size_t bytes total byte size will be written.
    , mResizePolicy(std::move(resizePolicy))
    , mData(new unsigned char[initialCapacity])
    , mMode(Mode::WRITE)
    , mInitialMode(Mode::WRITE)
{
}

BinaryContainer::BinaryContainer(char* data, size_t size)
    : mCapacity(size)
    , mCurrentPtrLocation(0)
    , mResizePolicy([](size_t)->size_t { assert(false); return 0; })
    , mData(reinterpret_cast<unsigned char*>(data))
    , mMode(Mode::READ)
    , mInitialMode(Mode::READ)
{
}


BinaryContainer::~BinaryContainer()
{
    if (mInitialMode == Mode::WRITE)
        delete[] mData;
}

void BinaryContainer::Resize()
{
    assert(mMode == Mode::WRITE);
    mCapacity = mResizePolicy(mCapacity);
    unsigned char* tmp = new unsigned char[mCapacity];
    memcpy(tmp, mData, mCurrentPtrLocation);
    std::swap(tmp, mData);
    delete[] tmp;
}

BinaryContainer& BinaryContainer::operator<<(int val)
{
    assert(mMode == Mode::WRITE);
    if (sizeof(val) + mCurrentPtrLocation > mCapacity)
        Resize();
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(float val)
{
    assert(mMode == Mode::WRITE);
    if (sizeof(val) + mCurrentPtrLocation > mCapacity)
        Resize();
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(UINT val)
{
    assert(mMode == Mode::WRITE);
    if (sizeof(val) + mCurrentPtrLocation > mCapacity)
        Resize();
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(size_t val)
{
    assert(mMode == Mode::WRITE);
    while (sizeof(val) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const std::vector<int>& val)
{
    assert(mMode == Mode::WRITE);
    while (sizeof(int) * val.size() + sizeof(size_t) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(int) * val.size();
    memcpy(p, val.data(), sizeof(int) * val.size());
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const std::vector<byte>& val)
{
    assert(mMode == Mode::WRITE);
    while (sizeof(int) * val.size() + sizeof(size_t) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(byte) * val.size();
    memcpy(p, val.data(), sizeof(byte) * val.size());
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const std::string& val)
{
    assert(mMode == Mode::WRITE);
    while (val.size() + sizeof(size_t) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += val.size();
    memcpy(p, val.data(), val.size());
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const std::pair<const unsigned char*, size_t>& val)
{
    assert(mMode == Mode::WRITE);
    while (val.second + sizeof(size_t) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.second);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += val.second;
    memcpy(p, val.first, val.second);
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const DirectX::XMFLOAT2& val)
{
    this->operator<<(val.x);
    this->operator<<(val.y);
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const DirectX::XMFLOAT3& val)
{
    this->operator<<(val.x);
    this->operator<<(val.y);
    this->operator<<(val.z);
    return *this;
}

BinaryContainer& BinaryContainer::operator<<(const DirectX::XMFLOAT4& val)
{
    this->operator<<(val.x);
    this->operator<<(val.y);
    this->operator<<(val.z);
    this->operator<<(val.w);
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(int& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(int);
    memcpy(&val, p, sizeof(int));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(float& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(float);
    memcpy(&val, p, sizeof(float));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(UINT& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(&val, p, sizeof(val));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(size_t& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(size_t);
    memcpy(&val, p, sizeof(size_t));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(std::vector<int>& val)
{
    assert(mMode == Mode::READ);
    assert(val.empty());
    size_t sz = 0;
    this->operator>>(sz);
    val.resize(sz);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sz * sizeof(int);
    memcpy(val.data(), p, sz * sizeof(int));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(std::vector<byte>& val)
{
    assert(mMode == Mode::READ);
    assert(val.empty());
    size_t sz = 0;
    this->operator>>(sz);
    val.resize(sz);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sz * sizeof(byte);
    memcpy(val.data(), p, sz * sizeof(byte));
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(std::string& val)
{
    assert(mMode == Mode::READ);
    assert(val.empty());
    size_t sz = 0;
    this->operator>>(sz);
    val.resize(sz / sizeof(unsigned char)); // TODO: deduce correct underlying type for string.
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sz;
    memcpy(val.data(), p, sz);
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(std::pair<unsigned char*, size_t>& val)
{
    assert(mMode == Mode::READ);
    assert(val.first == nullptr);
    this->operator>>(val.second);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += val.second;
    val.first = new unsigned char[val.second];
    memcpy(val.first, p, val.second);
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(DirectX::XMFLOAT2& val)
{
    this->operator>>(val.x);
    this->operator>>(val.y);
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(DirectX::XMFLOAT3& val)
{
    this->operator>>(val.x);
    this->operator>>(val.y);
    this->operator>>(val.z);
    return *this;
}

BinaryContainer& BinaryContainer::operator>>(DirectX::XMFLOAT4& val)
{
    this->operator>>(val.x);
    this->operator>>(val.y);
    this->operator>>(val.z);
    this->operator>>(val.w);
    return *this;
}
}
