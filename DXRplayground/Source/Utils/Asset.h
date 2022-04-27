#pragma once
#include <string>

namespace DirectxPlayground
{
class BinaryContainer;

class Asset
{
public:
    virtual void Parse(const std::string& filename) = 0;
    virtual void Serialize(BinaryContainer& container) = 0;
    virtual void Deserialize(BinaryContainer& container) = 0;
    virtual size_t GetVersion() const = 0;

    virtual ~Asset() = default;
};
}
