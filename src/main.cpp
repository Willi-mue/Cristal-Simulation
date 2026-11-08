#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>

#include "image_writer.hpp"
#include "options.hpp"
#include "scenario.hpp"
#include "simulation.hpp"
#ifdef CRISTAL_GUI
#include "viewer.hpp"
#endif

namespace fs = std::filesystem;

namespace {

// Creates `dir` and removes frames left over from a previous run.
bool prepare_frame_dir(const fs::path& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Cannot create " << dir.string() << ": " << ec.message() << "\n";
        return false;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_regular_file() && name.starts_with("frame_") &&
            (name.ends_with(".png") || name.ends_with(".ppm"))) {
            fs::remove(entry.path(), ec);
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parse_options(argc, argv, opt)) {
        return opt.help ? 0 : 1;
    }
    const std::uint64_t seed = opt.seed.value_or(std::random_device{}());

    if (opt.live) {
#ifdef CRISTAL_GUI
        return run_live(opt, seed);
#else
        std::cerr << "--live needs a build with the viewer: cmake --preset gui && cmake --build --preset gui\n";
        return 1;
#endif
    }

    const fs::path out_dir = opt.out_dir;
    if (!prepare_frame_dir(out_dir)) {
        return 1;
    }

    const int max_steps = effective_max_steps(opt);
    const auto start_time = std::chrono::steady_clock::now();

    Simulation sim = create_simulation(opt, seed);
    const std::size_t initial_crystals = sim.crystal_count();

    int frame = 0;
    auto save_frame = [&] {
        const fs::path file = out_dir / ("frame_" + std::to_string(frame++) + ".png");
        if (!write_png(sim, file.string())) {
            std::cerr << "Cannot write " << file.string() << "\n";
            return false;
        }
        return true;
    };

    // Frames are spread evenly over the run's progress: steps done (if limited)
    // or particles attached, whichever is further along.
    int step = 0;
    auto progress = [&] {
        double p = opt.particles > 0
                       ? static_cast<double>(sim.crystal_count() - initial_crystals) / opt.particles
                       : 1.0;
        if (max_steps > 0) {
            p = std::max(p, static_cast<double>(step) / max_steps);
        }
        return p;
    };

    if (opt.frames > 0 && !save_frame()) {
        return 1;
    }
    int frames_due = 0;
    bool last_saved = opt.frames > 0;
    while (!sim.finished() && (max_steps == 0 || step < max_steps)) {
        sim.step();
        step++;
        last_saved = false;
        const int due = static_cast<int>(progress() * opt.frames);
        if (due > frames_due) {
            frames_due = due;
            if (!save_frame()) {
                return 1;
            }
            last_saved = true;
        }
    }
    if (!last_saved && !save_frame()) {
        return 1;
    }

    const char* reason = sim.hit_border()      ? "cluster reached the border"
                         : sim.finished()      ? "no walkers left"
                                               : "step limit reached";
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start_time;
    std::cout << "Seed:     " << seed << "\n"
              << "Steps:    " << step << " (" << reason << ")\n"
              << "Crystal:  " << sim.crystal_count() << " cells, " << sim.walker_count() << " walkers left\n"
              << "Frames:   " << frame << " in " << out_dir.string() << "\n"
              << "Time:     " << elapsed.count() << " s\n";
    return 0;
}
