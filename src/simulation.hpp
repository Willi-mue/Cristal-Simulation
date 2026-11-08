#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

enum class Cell : std::uint8_t {
    Empty = 0,
    Crystal = 1,
    Walker = 2,
};

// Diffusion-limited aggregation on a square grid.
//
// Two ways to set it up:
//  - uniform: place() seeds and walkers at random positions, then step().
//  - radial:  place_center_seed() + enable_radial(); walkers are released on a
//             circle around the growing cluster, which gives classic DLA fractals.
class Simulation {
public:
    // stick: probability (0..1] that a walker touching the crystal attaches to it.
    Simulation(int size, std::uint64_t seed, double stick = 1.0);

    // Places up to `amount` cells of the given type (Crystal or Walker) on random
    // empty cells that do not touch the crystal. Returns how many were placed.
    int place(int amount, Cell type);

    // Puts a single crystal seed in the middle of the grid.
    void place_center_seed();

    // Radial mode: releases `budget` walkers in total, keeping up to `active` of
    // them on the grid at once. Walkers that wander too far are released again.
    void enable_radial(int budget, int active);

    // Moves every walker one step in a random direction; a walker that touches
    // the crystal (8-neighbourhood) afterwards attaches with probability `stick`.
    void step();

    // True when nothing can happen anymore: no walkers left to move or release,
    // or (radial mode) the cluster has grown up to the grid border.
    bool finished() const { return hit_border_ || (walkers_.empty() && budget_ == 0); }
    bool hit_border() const { return hit_border_; }

    int size() const { return size_; }
    std::size_t walker_count() const { return walkers_.size(); }
    std::size_t crystal_count() const { return crystals_; }
    const std::vector<Cell>& grid() const { return grid_; }

    // Order in which each crystal cell attached (0 = first seed). Only meaningful
    // for cells that are Crystal.
    const std::vector<std::uint32_t>& growth_order() const { return order_; }

private:
    int index(int x, int y) const { return y * size_ + x; }
    bool in_bounds(int x, int y) const { return x >= 0 && x < size_ && y >= 0 && y < size_; }
    bool touches_crystal(int i) const;
    void crystallize(int i);
    int distance_sq_from_center(int i) const;
    int spawn_on_circle();  // returns the new walker's index or -1

    int size_;
    std::vector<Cell> grid_;
    std::vector<std::uint32_t> order_;
    std::vector<int> walkers_;  // grid indices of all active walkers
    std::size_t crystals_ = 0;
    std::mt19937_64 rng_;
    double stick_;

    // radial mode
    bool radial_ = false;
    int budget_ = 0;
    int active_target_ = 0;
    int cluster_radius_ = 0;
    bool hit_border_ = false;
};
