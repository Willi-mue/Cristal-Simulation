#pragma once

#include <cstdint>

#include "options.hpp"

// Opens a window and shows the simulation growing in real time (needs CRISTAL_GUI).
// Returns the process exit code.
int run_live(const Options& opt, std::uint64_t seed);
