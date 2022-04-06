#pragma once

#include <functional>

namespace Kioto
{
class BinaryContainer
{
private:
    static size_t DoubleSize(size_t prevSize)
    {
        return prevSize * 2;
    }

public:
    BinaryContainer(size_t initialCapacity = 1024, std::function<size_t(size_t)> resizePolicy = [](size_t prevSize)->size_t { return DoubleSize(prevSize); });
    ~BinaryContainer();

private:
    size_t mCapacity;
    size_t mBytesWritten;
    std::function<size_t(size_t)> mResizePolicy;
    unsigned char* mData;

    void Resize();

public:
    BinaryContainer& operator<< (int val);
    BinaryContainer& operator<< (size_t val);
    BinaryContainer& operator<< (std::vector<int> val);
    BinaryContainer& operator<< (std::string val);
    BinaryContainer& operator<< (std::pair<unsigned char*, size_t> val);
};

inline BinaryContainer::BinaryContainer(size_t initialCapacity, std::function<size_t(size_t)> resizePolicy)
    : mCapacity(initialCapacity)
    , mBytesWritten(0)
    , mResizePolicy(std::move(resizePolicy))
    , mData(new unsigned char[initialCapacity])
{
}

inline BinaryContainer::~BinaryContainer()
{
    delete[] mData;
}

inline void BinaryContainer::Resize()
{
    mCapacity = mResizePolicy(mCapacity);
    unsigned char* tmp = new unsigned char[mCapacity];
    memcpy(tmp, mData, mBytesWritten);
    std::swap(tmp, mData);
    delete[] tmp;
}

inline BinaryContainer& BinaryContainer::operator<<(int val)
{
    if (sizeof(val) + mBytesWritten > mCapacity)
        Resize();
    unsigned char* p = mData + mBytesWritten;
    mBytesWritten += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(size_t val)
{
    while (sizeof(val) + mBytesWritten > mCapacity)
    {
        Resize();
    }
    unsigned char* p = mData + mBytesWritten;
    mBytesWritten += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(std::vector<int> val)
{
    while (sizeof(int) * val.size() + sizeof(size_t) + mBytesWritten > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mBytesWritten;
    mBytesWritten += sizeof(int) * val.size();
    memcpy(p, val.data(), sizeof(int) * val.size());
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(std::string val)
{
    while (val.size() + sizeof(size_t) + mBytesWritten > mCapacity)
    {
        Resize();
    }
    this->operator<<(val.size());
    unsigned char* p = mData + mBytesWritten;
    mBytesWritten += val.size();
    memcpy(p, val.data(), val.size());
    return *this;
}
}
