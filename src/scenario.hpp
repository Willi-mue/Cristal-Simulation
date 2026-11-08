#pragma once

#include <cstdint>

#include "options.hpp"
#include "simulation.hpp"

// Builds a ready-to-run simulation (seeds and walkers placed) for the given options.
Simulation create_simulation(const Options& opt, std::uint64_t seed);

// Step limit for the options: --steps if given, otherwise 2000 (uniform) or unlimited (radial). 0 = unlimited.
int effective_max_steps(const Options& opt);
