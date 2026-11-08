#include "simulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {

struct Offset {
    int dx;
    int dy;
};

constexpr std::array<Offset, 4> kDirections{{{0, -1}, {0, 1}, {-1, 0}, {1, 0}}};

// Radial mode: walkers start this many cells outside the cluster ...
constexpr int kSpawnMargin = 5;
// ... and are released again once they are farther away than this factor times the spawn radius.
constexpr int kKillFactor = 2;

}  // namespace

Simulation::Simulation(int size, std::uint64_t seed, double stick)
    : size_(size),
      grid_(static_cast<std::size_t>(size) * size, Cell::Empty),
      order_(grid_.size(), 0),
      rng_(seed),
      stick_(stick) {}

bool Simulation::touches_crystal(int i) const {
    const int x = i % size_;
    const int y = i / size_;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if ((dx != 0 || dy != 0) && in_bounds(x + dx, y + dy) &&
                grid_[index(x + dx, y + dy)] == Cell::Crystal) {
                return true;
            }
        }
    }
    return false;
}

int Simulation::distance_sq_from_center(int i) const {
    const int dx = i % size_ - size_ / 2;
    const int dy = i / size_ - size_ / 2;
    return dx * dx + dy * dy;
}

void Simulation::crystallize(int i) {
    grid_[i] = Cell::Crystal;
    order_[i] = static_cast<std::uint32_t>(crystals_++);

    if (radial_) {
        const int r = static_cast<int>(std::ceil(std::sqrt(distance_sq_from_center(i))));
        cluster_radius_ = std::max(cluster_radius_, r);
        if (cluster_radius_ + kSpawnMargin >= size_ / 2 - 1) {
            hit_border_ = true;
        }
    }
}

int Simulation::place(int amount, Cell type) {
    if (type == Cell::Empty || amount <= 0) {
        return 0;
    }

    std::vector<int> candidates;
    for (int i = 0; i < size_ * size_; i++) {
        if (grid_[i] == Cell::Empty && !touches_crystal(i)) {
            candidates.push_back(i);
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), rng_);

    const int placed = std::min(amount, static_cast<int>(candidates.size()));
    for (int k = 0; k < placed; k++) {
        if (type == Cell::Crystal) {
            crystallize(candidates[k]);
        } else {
            grid_[candidates[k]] = Cell::Walker;
            walkers_.push_back(candidates[k]);
        }
    }
    return placed;
}

void Simulation::place_center_seed() {
    crystallize(index(size_ / 2, size_ / 2));
}

void Simulation::enable_radial(int budget, int active) {
    radial_ = true;
    budget_ = std::max(budget, 0);
    active_target_ = std::max(active, 1);
}

int Simulation::spawn_on_circle() {
    std::uniform_real_distribution<double> angle(0.0, 2.0 * std::numbers::pi);
    const double radius = cluster_radius_ + kSpawnMargin;
    for (int attempt = 0; attempt < 32; attempt++) {
        const double a = angle(rng_);
        const int x = size_ / 2 + static_cast<int>(std::lround(radius * std::cos(a)));
        const int y = size_ / 2 + static_cast<int>(std::lround(radius * std::sin(a)));
        if (in_bounds(x, y) && grid_[index(x, y)] == Cell::Empty && !touches_crystal(index(x, y))) {
            grid_[index(x, y)] = Cell::Walker;
            return index(x, y);
        }
    }
    return -1;
}

void Simulation::step() {
    if (radial_) {
        while (!hit_border_ && budget_ > 0 && static_cast<int>(walkers_.size()) < active_target_) {
            const int i = spawn_on_circle();
            if (i < 0) {
                break;
            }
            walkers_.push_back(i);
            budget_--;
        }
    }

    // Random processing order so no walker is systematically favoured.
    std::shuffle(walkers_.begin(), walkers_.end(), rng_);
    std::uniform_int_distribution<std::size_t> pick_direction(0, kDirections.size() - 1);
    std::bernoulli_distribution sticks(std::clamp(stick_, 0.0, 1.0));

    const int kill_radius = std::min(kKillFactor * (cluster_radius_ + kSpawnMargin), size_ / 2 - 1);
    const int kill_radius_sq = kill_radius * kill_radius;

    std::size_t k = 0;
    while (k < walkers_.size()) {
        int i = walkers_[k];
        const Offset dir = kDirections[pick_direction(rng_)];
        const int nx = i % size_ + dir.dx;
        const int ny = i / size_ + dir.dy;

        // Walkers stay in place when blocked by the border or another cell.
        if (in_bounds(nx, ny) && grid_[index(nx, ny)] == Cell::Empty) {
            grid_[i] = Cell::Empty;
            i = index(nx, ny);
            grid_[i] = Cell::Walker;
            walkers_[k] = i;
        }

        if (radial_ && distance_sq_from_center(i) > kill_radius_sq) {
            grid_[i] = Cell::Empty;
            const int respawned = spawn_on_circle();
            if (respawned >= 0) {
                walkers_[k] = respawned;
                k++;
            } else {
                budget_++;  // give it back, it will be released later
                walkers_[k] = walkers_.back();
                walkers_.pop_back();
            }
            continue;
        }

        if (touches_crystal(i) && sticks(rng_)) {
            crystallize(i);
            walkers_[k] = walkers_.back();  // swap-remove; the moved walker is handled next
            walkers_.pop_back();
        } else {
            k++;
        }
    }
}
