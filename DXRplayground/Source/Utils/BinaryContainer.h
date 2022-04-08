#pragma once

#include <functional>

namespace DirectxPlayground
{
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
    , mCurrentPtrLocation(sizeof(size_t))
    , mResizePolicy(std::move(resizePolicy))
    , mData(new unsigned char[initialCapacity])
    , mMode(Mode::WRITE)
    , mInitialMode(Mode::WRITE)
{
}

inline BinaryContainer::BinaryContainer(char* data, size_t size)
    : mCapacity(size)
    , mCurrentPtrLocation(0)
    , mResizePolicy([](size_t)->size_t { assert(false); return 0; })
    , mData(reinterpret_cast<unsigned char*>(data))
    , mMode(Mode::READ)
    , mInitialMode(Mode::READ)
{
}


inline BinaryContainer::~BinaryContainer()
{
    if (mInitialMode == Mode::WRITE)
        delete[] mData;
}

inline void BinaryContainer::Resize()
{
    assert(mMode == Mode::WRITE);
    mCapacity = mResizePolicy(mCapacity);
    unsigned char* tmp = new unsigned char[mCapacity];
    memcpy(tmp, mData, mCurrentPtrLocation);
    std::swap(tmp, mData);
    delete[] tmp;
}

inline BinaryContainer& BinaryContainer::operator<<(int val)
{
    assert(mMode == Mode::WRITE);
    if (sizeof(val) + mCurrentPtrLocation > mCapacity)
        Resize();
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(val);
    memcpy(p, &val, sizeof(val));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator<<(size_t val)
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

inline BinaryContainer& BinaryContainer::operator<<(const std::vector<int>& val)
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

inline BinaryContainer& BinaryContainer::operator<<(const std::string& val)
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

inline BinaryContainer& BinaryContainer::operator<<(const std::pair<unsigned char*, size_t>& val)
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

inline BinaryContainer& BinaryContainer::operator>>(int& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(int);
    memcpy(&val, p, sizeof(int));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator>>(size_t& val)
{
    assert(mMode == Mode::READ);
    unsigned char* p = mData + mCurrentPtrLocation;
    mCurrentPtrLocation += sizeof(size_t);
    memcpy(&val, p, sizeof(size_t));
    return *this;
}

inline BinaryContainer& BinaryContainer::operator>>(std::vector<int>& val)
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

inline BinaryContainer& BinaryContainer::operator>>(std::string& val)
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

inline BinaryContainer& BinaryContainer::operator>>(std::pair<unsigned char*, size_t>& val)
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

struct Structololosha
{
public:
    int a = 0, b = 0;
    friend BinaryContainer& operator<<(BinaryContainer& op, const Structololosha& s)
    {
        op << s.a << s.b;
        return op;
    }
    friend BinaryContainer& operator>>(BinaryContainer& op, Structololosha& s)
    {
        op >> s.a >> s.b;
        return op;
    }
};

}
