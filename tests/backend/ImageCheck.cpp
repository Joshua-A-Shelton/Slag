#include "ImageCheck.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <cstring>
#include <fstream>

namespace {

constexpr int channelTolerance = 2;
constexpr double pixelTolerance = 0.005;

std::string goldenPath(const std::string& name, const char* suffix)
{
    return std::string(SLAG_GOLDEN_DIR "/") + name + suffix;
}

bool readImage(const std::string& path, Image& image)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }
    uint32_t header[2] = {};
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    image.width = header[0];
    image.height = header[1];
    image.rgba.resize(size_t(image.width) * image.height * 4);
    file.read(reinterpret_cast<char*>(image.rgba.data()), static_cast<std::streamsize>(image.rgba.size()));
    return static_cast<bool>(file);
}

void writeImage(const std::string& path, const Image& image)
{
    std::ofstream file(path, std::ios::binary);
    uint32_t header[2] = {image.width, image.height};
    file.write(reinterpret_cast<const char*>(header), sizeof(header));
    file.write(reinterpret_cast<const char*>(image.rgba.data()), static_cast<std::streamsize>(image.rgba.size()));
}

} // namespace

uint32_t Image::pixel(uint32_t x, uint32_t y) const
{
    const uint8_t* p = rgba.data() + (size_t(y) * width + x) * 4;
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

uint32_t checkExact(const Image& image, const std::function<std::optional<Rgba>(uint32_t x, uint32_t y)>& expected)
{
    uint32_t mismatches = 0;
    uint32_t checked = 0;
    for (uint32_t y = 0; y < image.height; ++y)
    {
        for (uint32_t x = 0; x < image.width; ++x)
        {
            std::optional<Rgba> value = expected(x, y);
            if (!value)
            {
                continue;
            }
            ++checked;
            if (image.pixel(x, y) != *value)
            {
                if (mismatches < 5)
                {
                    char text[128];
                    std::snprintf(text, sizeof(text), "pixel (%u, %u) is %08x, expected %08x", x, y, image.pixel(x, y),
                                  *value);
                    MESSAGE(text);
                }
                ++mismatches;
            }
        }
    }
    MESSAGE("exact check: " << checked << " pixels, " << mismatches << " mismatches");
    return mismatches;
}

bool imagesMatch(const Image& a, const Image& b, std::string& why)
{
    if (a.width != b.width || a.height != b.height)
    {
        why = "sizes differ";
        return false;
    }
    size_t off = 0;
    int largest = 0;
    for (size_t i = 0; i < a.rgba.size(); i += 4)
    {
        int worst = 0;
        for (size_t c = 0; c < 4; ++c)
        {
            int difference = std::abs(int(a.rgba[i + c]) - int(b.rgba[i + c]));
            worst = difference > worst ? difference : worst;
        }
        largest = worst > largest ? worst : largest;
        off += worst > channelTolerance ? 1 : 0;
    }
    size_t pixels = a.rgba.size() / 4;
    why = std::to_string(off) + " of " + std::to_string(pixels) + " pixels differ by more than " +
          std::to_string(channelTolerance) + " (largest difference " + std::to_string(largest) + ")";
    return double(off) <= pixelTolerance * double(pixels);
}

bool checkGolden(const Image& image, const std::string& name)
{
    const char* update = std::getenv("SLAG_UPDATE_GOLDEN");
    if (update && update[0] == '1')
    {
        writeImage(goldenPath(name, ".rgba"), image);
        MESSAGE("wrote golden " << goldenPath(name, ".rgba"));
        return true;
    }
    Image golden;
    if (!readImage(goldenPath(name, ".rgba"), golden))
    {
        writeImage(goldenPath(name, ".actual.rgba"), image);
        MESSAGE("no golden " << goldenPath(name, ".rgba") << "; run with SLAG_UPDATE_GOLDEN=1 to create it");
        return false;
    }
    std::string why;
    bool match = imagesMatch(golden, image, why);
    MESSAGE("golden check " << name << ": " << why);
    if (!match)
    {
        writeImage(goldenPath(name, ".actual.rgba"), image);
    }
    return match;
}
