#include "options.hpp"

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_help() {
    const Options d;
    std::cout << "Usage: cristal [options]\n"
                 "\n"
                 "Diffusion-limited aggregation (DLA) crystal growth simulation.\n"
                 "\n"
                 "Options:\n"
                 "  --mode M         uniform | radial                        (default uniform)\n"
                 "                   uniform: seeds and walkers spread over the grid\n"
                 "                   radial:  one seed in the centre, branching DLA fractal\n"
                 "  --size N         grid width and height in cells          (default " << d.size << ")\n"
                 "  --particles N    number of walkers                       (default " << d.particles << ")\n"
                 "  --seeds N        initial crystal seeds, uniform mode     (default " << d.seeds << ")\n"
                 "  --active N       walkers on the grid at once, radial     (default " << d.active << ")\n"
                 "  --stick P        attach probability on contact, 0..1     (default " << d.stick << ")\n"
                 "  --steps N        step limit, 0 = unlimited               (default 2000 uniform, 0 radial)\n"
                 "  --frames N       number of frames to write, 0 = final only (default " << d.frames << ")\n"
                 "  --seed N         random seed for reproducible runs       (default: random)\n"
                 "  --out DIR        directory for the frames                (default " << d.out_dir << ")\n"
                 "  --live           show the growth in a window instead of writing frames\n"
                 "                   (needs a build with -DCRISTAL_GUI=ON, e.g. preset 'gui')\n"
                 "  -h, --help       show this help\n";
}

template <typename T>
bool parse_number(std::string_view text, T& value) {
    const char* end = text.data() + text.size();
    auto [ptr, ec] = std::from_chars(text.data(), end, value);
    return ec == std::errc{} && ptr == end;
}

bool parse_double(std::string_view text, double& value) {
    const std::string s(text);
    char* end = nullptr;
    value = std::strtod(s.c_str(), &end);
    return !s.empty() && end == s.c_str() + s.size();
}

}  // namespace

bool parse_options(int argc, char** argv, Options& opt) {
    for (int a = 1; a < argc; a++) {
        const std::string_view arg = argv[a];
        if (arg == "-h" || arg == "--help") {
            print_help();
            opt.help = true;
            return false;
        }
        if (arg == "--live") {
            opt.live = true;
            continue;
        }

        const bool known = arg == "--mode" || arg == "--size" || arg == "--particles" ||
                           arg == "--seeds" || arg == "--active" || arg == "--stick" ||
                           arg == "--steps" || arg == "--frames" || arg == "--seed" || arg == "--out";
        if (!known) {
            std::cerr << "Unknown option: " << arg << " (see --help)\n";
            return false;
        }
        if (a + 1 >= argc) {
            std::cerr << "Missing value for " << arg << "\n";
            return false;
        }
        const std::string_view value = argv[++a];

        bool ok = true;
        if (arg == "--mode") {
            ok = value == "uniform" || value == "radial";
            opt.mode = value == "radial" ? Mode::Radial : Mode::Uniform;
        } else if (arg == "--size") {
            ok = parse_number(value, opt.size);
        } else if (arg == "--particles") {
            ok = parse_number(value, opt.particles);
        } else if (arg == "--seeds") {
            ok = parse_number(value, opt.seeds);
        } else if (arg == "--active") {
            ok = parse_number(value, opt.active);
        } else if (arg == "--stick") {
            ok = parse_double(value, opt.stick);
        } else if (arg == "--steps") {
            int steps = 0;
            ok = parse_number(value, steps);
            opt.max_steps = steps;
        } else if (arg == "--frames") {
            ok = parse_number(value, opt.frames);
        } else if (arg == "--seed") {
            std::uint64_t seed = 0;
            ok = parse_number(value, seed);
            opt.seed = seed;
        } else if (arg == "--out") {
            opt.out_dir = std::string(value);
        }
        if (!ok) {
            std::cerr << "Invalid value for " << arg << ": " << value << "\n";
            return false;
        }
    }

    if (opt.size < 3 || opt.size > 20000) {
        std::cerr << "--size must be between 3 and 20000\n";
        return false;
    }
    if (!(opt.stick > 0.0 && opt.stick <= 1.0)) {
        std::cerr << "--stick must be greater than 0 and at most 1\n";
        return false;
    }
    if (opt.particles < 0 || opt.seeds < 1 || opt.active < 1 || opt.frames < 0 ||
        opt.max_steps.value_or(0) < 0) {
        std::cerr << "--particles, --steps and --frames must be >= 0, --seeds and --active >= 1\n";
        return false;
    }
    return true;
}
