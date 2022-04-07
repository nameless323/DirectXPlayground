#pragma once

#include <functional>

namespace DirectxPlayground
{
class BinaryContainer
{
public:
    static size_t DoubleSize(size_t prevSize)
    {
        return prevSize * 2;
    }

    BinaryContainer(size_t initialCapacity = 1024, std::function<size_t(size_t)> resizePolicy = [](size_t prevSize)->size_t { return BinaryContainer::DoubleSize(prevSize); });
    BinaryContainer(unsigned char* data, size_t size);
    ~BinaryContainer();

private:
    enum class Mode
    {
        READ,
        WRITE
    };

    size_t mCapacity;
    size_t mCurrentPtrLocation;
    std::function<size_t(size_t)> mResizePolicy;
    unsigned char* mData;
    const Mode mMode;

    void Resize();

public:
    BinaryContainer& operator<< (int val);
    BinaryContainer& operator<< (size_t val);
    BinaryContainer& operator<< (const std::vector<int>& val);
    BinaryContainer& operator<< (const std::string& val);
    BinaryContainer& operator<< (const std::pair<unsigned char*, size_t>& val);

    BinaryContainer& operator>> (int& val);
    BinaryContainer& operator>> (size_t& val);
    BinaryContainer& operator>> (std::vector<int>& val);
    BinaryContainer& operator>> (std::string& val);
    BinaryContainer& operator>> (std::pair<unsigned char*, size_t>& val);
};

inline BinaryContainer::BinaryContainer(size_t initialCapacity, std::function<size_t(size_t)> resizePolicy)
    : mCapacity(initialCapacity)
    , mCurrentPtrLocation(0)
    , mResizePolicy(std::move(resizePolicy))
    , mData(new unsigned char[initialCapacity])
    , mMode(Mode::WRITE)
{
}

inline BinaryContainer::BinaryContainer(unsigned char* data, size_t size)
    : mCapacity(size)
    , mCurrentPtrLocation(0)
    , mResizePolicy([](size_t)->size_t { assert(false); return 0; })
    , mData(data)
    , mMode(Mode::READ)
{
}


inline BinaryContainer::~BinaryContainer()
{
    if (mMode == Mode::WRITE)
        delete[] mData;
}

inline void BinaryContainer::Resize()
{
    mCapacity = mResizePolicy(mCapacity);
    unsigned char* tmp = new unsigned char[mCapacity];
    memcpy(tmp, mData, mCurrentPtrLocation);
    std::swap(tmp, mData);
    delete[] tmp;
}

inline BinaryContainer& BinaryContainer::operator<<(int val)
{
    if (sizeof(val) + mCurrentPtrLocation > mCapacity)
        Resize();
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(size_t val)
{
    while (sizeof(val) + mCurrentPtrLocation > mCapacity)
    {
        Resize();
    }
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(const std::vector<int>& val)
{
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

inline BinaryContainer& BinaryContainer::operator<<(const std::string& val)
{
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

inline BinaryContainer& BinaryContainer::operator<<(const std::pair<unsigned char*, size_t>& val)
{
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

struct Structololosha
{
public:
    int a = 0, b = 0;
    friend BinaryContainer& operator<<(BinaryContainer& op, const Structololosha& s)
    {
        op << s.a << s.b;
        return op;
    }
};

}
