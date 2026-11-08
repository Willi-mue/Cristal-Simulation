#include "image_writer.hpp"

#include <algorithm>
#include <array>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC  // keep symbols local: raylib (live viewer) bundles its own copy
#define STBIW_WINDOWS_UTF8      // file names are UTF-8 on Windows too
#include <stb_image_write.h>

namespace {

struct Rgb {
    unsigned char r, g, b;
};

constexpr Rgb kBackground{12, 13, 24};  // near-black blue
constexpr Rgb kWalker{70, 82, 120};     // dim blue-grey, visible but not distracting

// Crystal colour by age: oldest cells (core) glow pale yellow, the newest
// (outer branches) fade through orange and crimson to purple.
constexpr std::array<Rgb, 5> kCrystalGradient{{
    {252, 255, 164},
    {250, 193, 39},
    {237, 105, 37},
    {188, 55, 84},
    {120, 28, 109},
}};

Rgb gradient(double t) {
    t = std::clamp(t, 0.0, 1.0) * (kCrystalGradient.size() - 1);
    const std::size_t lo = std::min(static_cast<std::size_t>(t), kCrystalGradient.size() - 2);
    const double f = t - static_cast<double>(lo);
    const Rgb& a = kCrystalGradient[lo];
    const Rgb& b = kCrystalGradient[lo + 1];
    auto mix = [f](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(x + (y - x) * f + 0.5);
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b)};
}

}  // namespace

std::vector<unsigned char> render_rgb(const Simulation& sim) {
    const auto& grid = sim.grid();
    const auto& order = sim.growth_order();
    const double newest = std::max<double>(1.0, static_cast<double>(sim.crystal_count()) - 1.0);

    std::vector<unsigned char> pixels;
    pixels.reserve(grid.size() * 3);
    for (std::size_t i = 0; i < grid.size(); ++i) {
        Rgb c = kBackground;
        if (grid[i] == Cell::Crystal) {
            c = gradient(order[i] / newest);
        } else if (grid[i] == Cell::Walker) {
            c = kWalker;
        }
        pixels.insert(pixels.end(), {c.r, c.g, c.b});
    }
    return pixels;
}

bool write_png(const Simulation& sim, const std::string& filename) {
    const std::vector<unsigned char> pixels = render_rgb(sim);
    const int n = sim.size();
    return stbi_write_png(filename.c_str(), n, n, 3, pixels.data(), n * 3) != 0;
}
