#pragma once

#include <string>

namespace DirectxPlayground
{
class Asset;

namespace AssetSystem
{
void Load(const std::string& assetPath, Asset& asset);
}
}
