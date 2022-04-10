#pragma once

#include <cassert>
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
