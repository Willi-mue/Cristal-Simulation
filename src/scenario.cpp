#include "scenario.hpp"

#include <iostream>

Simulation create_simulation(const Options& opt, std::uint64_t seed) {
    Simulation sim(opt.size, seed, opt.stick);
    if (opt.mode == Mode::Radial) {
        sim.place_center_seed();
        sim.enable_radial(opt.particles, opt.active);
    } else {
        const int seeds = sim.place(opt.seeds, Cell::Crystal);
        const int walkers = sim.place(opt.particles, Cell::Walker);
        if (seeds < opt.seeds || walkers < opt.particles) {
            std::cerr << "Warning: grid too small, placed " << seeds << " seeds and " << walkers << " walkers\n";
        }
    }
    return sim;
}

int effective_max_steps(const Options& opt) {
    return opt.max_steps.value_or(opt.mode == Mode::Radial ? 0 : 2000);
}
