#pragma once

#include <cstdint>
#include <optional>
#include <string>

enum class Mode {
    Uniform,  // seeds and walkers spread over the whole grid
    Radial,   // one seed in the centre, walkers released around the cluster (classic DLA)
};

struct Options {
    Mode mode = Mode::Uniform;
    int size = 320;
    int particles = 5000;
    int seeds = 6;                 // uniform mode only
    int active = 200;              // radial mode: walkers on the grid at the same time
    double stick = 1.0;
    std::optional<int> max_steps;  // default: 2000 (uniform), unlimited (radial); 0 = unlimited
    int frames = 500;              // 0 = only write the final frame
    std::optional<std::uint64_t> seed;
    std::string out_dir = "out/frames";
    bool live = false;  // show a window instead of writing frames
    bool help = false;
};

// Parses command line arguments into `opt`. Returns false if the program should
// exit: either `--help` was given (opt.help is set) or the arguments are invalid.
bool parse_options(int argc, char** argv, Options& opt);
