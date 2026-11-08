#pragma once

#include <string>
#include <vector>

#include "simulation.hpp"

// Renders the grid to 8-bit RGB pixels (row-major, 3 bytes per cell).
// Crystal cells are coloured by the order in which they attached.
std::vector<unsigned char> render_rgb(const Simulation& sim);

// Writes the simulation as a PNG image. Returns false if the file could not be written.
bool write_png(const Simulation& sim, const std::string& filename);
