#pragma once

#include "Utils/Asset.h"

#include <filesystem>
#include <fstream>

#include "BinaryContainer.h"

namespace DirectxPlayground::AssetSystem
{
inline void ParseAsset(const std::string& assetPath, Asset& asset, std::filesystem::path binAssetPath)
{
    BinaryContainer container{};
    container << size_t(0); // Reserved for timestamp

    asset.Parse(assetPath);
    asset.Serialize(container);

    std::chrono::high_resolution_clock::time_point timeNow = std::chrono::high_resolution_clock::now();
    size_t timeNowSeconds = static_cast<size_t>(std::chrono::duration_cast<std::chrono::seconds>(timeNow.time_since_epoch()).count());
    container.Write(sizeof(size_t), reinterpret_cast<char*>(&timeNowSeconds), sizeof(timeNowSeconds)); // First size_t bytes are reserved for the filesize, hence the offset.

    std::ofstream outFile(binAssetPath.string(), std::ios::out | std::ios::binary);
    outFile.write(container.GetData(), container.GetLastPointerOffset());
    outFile.close();
}

inline void Load(const std::string& assetPath, Asset& asset)
{
    // <assetPath> == \Repos\DXRplayground\DXRplayground\Assets\Models\FlightHelmet\glTF\FlightHelmet.gltf
    using namespace std;
    using namespace std::chrono;

    filesystem::path currentAssetPath{ assetPath };

    std::filesystem::path filename = currentAssetPath.stem(); // FlightHelmet
    std::filesystem::path projectAssetsPath{ ASSETS_DIR }; // \Repos\DXRplayground\DXRplayground\Assets
    std::filesystem::path relative = std::filesystem::relative(assetPath, projectAssetsPath); // Models\FlightHelmet\glTF\FlightHelmet.gltf
    std::filesystem::path parentPath = projectAssetsPath.parent_path().parent_path(); // <-- due to // in assets assetPath. \Repos\DXRplayground\DXRplayground (to create tmp there)
    //auto assetPath = ASSETS_DIR + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");
    std::filesystem::path binAssetPath{ parentPath.string() + std::string("//tmp//") + relative.string() }; // <---- extension Binary path
    if (!std::filesystem::exists(binAssetPath))
    {
        std::filesystem::path parentDir = binAssetPath.parent_path();
        if (!std::filesystem::exists(parentDir))
        {
            std::filesystem::create_directories(parentDir);
        }
        ParseAsset(assetPath, asset, binAssetPath);
    }
    else
    {
        filesystem::file_time_type lastModificationTime = filesystem::last_write_time(binAssetPath);
        seconds lmtSinceEpoch = std::chrono::duration_cast<seconds>(lastModificationTime.time_since_epoch());

        std::ifstream inFile(binAssetPath.string(), std::ios::in | std::ios::binary);
        size_t byteSize = 0;
        inFile.read((char*)&byteSize, sizeof(std::size_t));
        char* buf = new char[byteSize];
        inFile.read(buf, byteSize);
        inFile.close();

        BinaryContainer inContainer{ buf, byteSize };
        size_t lastBinModificationTime = 0;
        inContainer >> lastBinModificationTime;
        if (static_cast<size_t>(lmtSinceEpoch.count()) <= lastBinModificationTime)
        {
            asset.Deserialize(inContainer);
        }
        else
        {
            ParseAsset(assetPath, asset, binAssetPath);
        }

    }
}
}
