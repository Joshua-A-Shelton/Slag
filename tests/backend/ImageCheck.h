#pragma once

// The M3 image checks (docs/milestones.md): exact checks of pixels whose
// value the test knows, and a tolerance check against a committed golden
// image. Goldens are raw: 8-byte header (width, height as uint32), then RGBA8
// rows. Set SLAG_UPDATE_GOLDEN=1 to write them from the current results.

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct Image
{
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba; // tightly packed rows

    uint32_t pixel(uint32_t x, uint32_t y) const;
};

using Rgba = uint32_t; // r | g << 8 | b << 16 | a << 24

// Pixels where `expected` returns a value must match it exactly; others are
// skipped (edges, texel boundaries). Returns the number of mismatches and
// reports the first few.
uint32_t checkExact(const Image& image, const std::function<std::optional<Rgba>(uint32_t x, uint32_t y)>& expected);

// Compares against tests/golden/<name>.rgba: a channel may differ by at most
// 2, and at most 0.5% of pixels may differ more. On failure, writes
// <name>.actual.rgba next to the golden. Returns true on a match.
bool checkGolden(const Image& image, const std::string& name);

// Same tolerance, between two images (e.g. two backends).
bool imagesMatch(const Image& a, const Image& b, std::string& why);
