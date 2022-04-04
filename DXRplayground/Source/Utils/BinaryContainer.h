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
    BinaryContainer& operator<< (std::vector<int> val);
    BinaryContainer& operator<< (std::string val);
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


}
