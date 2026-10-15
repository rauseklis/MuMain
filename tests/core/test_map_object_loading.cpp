#include "stdafx.h"
#include "doctest.h"
#include "Engine/Object/ZzzObject.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>

TEST_CASE("Malformed Arena model indices do not enter the object list [world][objects]")
{
    // The original Arena contains model index -515. Loading it must not create
    // an object that later indexes before the model array during rendering.
    const auto path = std::filesystem::temp_directory_path()
        / ("mu-invalid-model-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj");
    std::array<OBJECT*, 256> originalHeads;
    for (std::size_t block = 0; block < originalHeads.size(); ++block)
        originalHeads[block] = ObjectBlock[block].Head;

    const unsigned char version = 0;
    const short count = 2;
    const std::array<short, 2> invalidTypes = {-515, static_cast<short>(MAX_MODELS)};
    const std::array<float, 7> placement = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
    {
        std::ofstream file(path, std::ios::binary);
        REQUIRE(file.is_open());
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const short type : invalidTypes)
        {
            file.write(reinterpret_cast<const char*>(&type), sizeof(type));
            file.write(reinterpret_cast<const char*>(placement.data()), sizeof(placement));
        }
    }
    auto filename = path.wstring();
    const int map = OpenObjects(filename.data());
    std::filesystem::remove(path);
    CHECK(map == 0);
    for (std::size_t block = 0; block < originalHeads.size(); ++block)
        CHECK(ObjectBlock[block].Head == originalHeads[block]);
}
