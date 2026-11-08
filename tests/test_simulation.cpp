// Minimal self-contained tests (no framework): run via `ctest`.
#include <cstdlib>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "image_writer.hpp"
#include "simulation.hpp"

namespace {

int failures = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
            failures++;                                                               \
        }                                                                             \
    } while (0)

std::size_t count(const Simulation& sim, Cell type) {
    std::size_t n = 0;
    for (Cell c : sim.grid()) {
        n += c == type;
    }
    return n;
}

// True if all crystal cells form one 8-connected cluster.
bool crystal_connected(const Simulation& sim) {
    const int n = sim.size();
    const auto& grid = sim.grid();
    std::vector<bool> seen(grid.size(), false);
    int start = -1;
    for (int i = 0; i < n * n; i++) {
        if (grid[i] == Cell::Crystal) {
            start = i;
            break;
        }
    }
    if (start < 0) {
        return true;
    }
    std::queue<int> todo;
    todo.push(start);
    seen[start] = true;
    std::size_t reached = 0;
    while (!todo.empty()) {
        const int i = todo.front();
        todo.pop();
        reached++;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                const int x = i % n + dx;
                const int y = i / n + dy;
                if (x >= 0 && x < n && y >= 0 && y < n && !seen[y * n + x] && grid[y * n + x] == Cell::Crystal) {
                    seen[y * n + x] = true;
                    todo.push(y * n + x);
                }
            }
        }
    }
    return reached == count(sim, Cell::Crystal);
}

void test_uniform_conserves_particles() {
    Simulation sim(64, 1);
    const int seeds = sim.place(3, Cell::Crystal);
    const int walkers = sim.place(500, Cell::Walker);
    CHECK(seeds == 3);
    CHECK(walkers == 500);
    for (int s = 0; s < 300; s++) {
        sim.step();
        CHECK(count(sim, Cell::Crystal) + count(sim, Cell::Walker) == 503u);
        CHECK(count(sim, Cell::Walker) == sim.walker_count());
        CHECK(count(sim, Cell::Crystal) == sim.crystal_count());
    }
}

void test_same_seed_same_result() {
    auto run = [](std::uint64_t seed) {
        Simulation sim(48, seed, 0.5);
        sim.place(2, Cell::Crystal);
        sim.place(300, Cell::Walker);
        for (int s = 0; s < 200; s++) {
            sim.step();
        }
        return render_rgb(sim);
    };
    CHECK(run(7) == run(7));
    CHECK(run(7) != run(8));
}

void test_tiny_grid() {
    Simulation sim(3, 1);
    sim.place_center_seed();
    CHECK(sim.place(1, Cell::Crystal) == 0);   // every free cell touches the centre seed
    CHECK(sim.place(100, Cell::Walker) == 0);
    CHECK(sim.finished());
    sim.step();  // must not crash
}

void test_radial_grows_one_connected_cluster() {
    Simulation sim(128, 3);
    sim.place_center_seed();
    sim.enable_radial(400, 50);
    int steps = 0;
    while (!sim.finished() && steps < 2'000'000) {
        sim.step();
        steps++;
    }
    CHECK(sim.finished());
    CHECK(sim.walker_count() == 0 || sim.hit_border());
    CHECK(sim.crystal_count() == count(sim, Cell::Crystal));
    CHECK(sim.hit_border() || sim.crystal_count() == 401u);
    CHECK(crystal_connected(sim));
}

void test_radial_stops_at_border() {
    Simulation sim(32, 5);
    sim.place_center_seed();
    sim.enable_radial(100000, 20);
    int steps = 0;
    while (!sim.finished() && steps < 2'000'000) {
        sim.step();
        steps++;
    }
    CHECK(sim.hit_border());
    CHECK(crystal_connected(sim));
}

}  // namespace

int main() {
    test_uniform_conserves_particles();
    test_same_seed_same_result();
    test_tiny_grid();
    test_radial_grows_one_connected_cluster();
    test_radial_stops_at_border();

    if (failures > 0) {
        std::cerr << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All tests passed\n";
    return EXIT_SUCCESS;
}
