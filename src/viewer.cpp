#include "viewer.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "image_writer.hpp"
#include "scenario.hpp"

namespace {

constexpr int kWindowSize = 800;
constexpr int kTopHeight = 48;
constexpr int kStatusHeight = 64;
constexpr int kMaxStepsPerFrame = 1 << 16;
constexpr float kValueSize = 20.0f;
constexpr float kLabelSize = 13.0f;
constexpr float kButtonTextSize = 16.0f;
constexpr Color kButton{35, 38, 60, 255};
constexpr Color kButtonHover{52, 56, 88, 255};
constexpr Color kBackground{12, 13, 24, 255};
constexpr Color kStatusBar{20, 22, 38, 255};
constexpr Color kValue{225, 228, 240, 255};
constexpr Color kLabel{120, 125, 150, 255};
constexpr Color kAccent{250, 193, 39, 255};

// Monospace system fonts, first one found wins. Monospace digits all have the
// same width, so changing numbers do not make the text jump around.
constexpr std::array kFontCandidates{
    "C:/Windows/Fonts/CascadiaMono.ttf",
    "C:/Windows/Fonts/consola.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/Library/Fonts/Courier New.ttf",
};

Font load_ui_font() {
    for (const char* path : kFontCandidates) {
        if (FileExists(path)) {
            // Load larger than drawn and filter bilinear: smooth at any size.
            Font font = LoadFontEx(path, 48, nullptr, 0);
            if (font.texture.id != 0) {
                SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
                return font;
            }
        }
    }
    return GetFontDefault();
}

// 81300 -> "81 300"
std::string group_digits(long long value) {
    std::string digits = std::to_string(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) {
        digits.insert(static_cast<std::size_t>(i), " ");
    }
    return digits;
}

// One status column: small label on top, value below, right-aligned to `right`.
void draw_field(const Font& font, float right, float top, const char* label, const std::string& value, Color color) {
    const Vector2 label_size = MeasureTextEx(font, label, kLabelSize, 1.0f);
    const Vector2 value_size = MeasureTextEx(font, value.c_str(), kValueSize, 1.0f);
    DrawTextEx(font, label, {right - label_size.x, top}, kLabelSize, 1.0f, kLabel);
    DrawTextEx(font, value.c_str(), {right - value_size.x, top + kLabelSize + 3.0f}, kValueSize, 1.0f, color);
}

// Toggle button; returns true when clicked.
bool button(const Font& font, Rectangle rect, const char* text, bool active) {
    const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    const Color fill = active ? kAccent : hover ? kButtonHover : kButton;
    DrawRectangleRounded(rect, 0.3f, 6, fill);
    const Vector2 size = MeasureTextEx(font, text, kButtonTextSize, 1.0f);
    DrawTextEx(font, text, {rect.x + (rect.width - size.x) / 2.0f, rect.y + (rect.height - size.y) / 2.0f},
               kButtonTextSize, 1.0f, active ? kBackground : kValue);
    if (hover && !active) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// In the live view the run never stops because of a limit meant for videos:
// no step limit (unless --steps is given) and radial mode grows until the border.
Options live_options(const Options& opt, Mode mode) {
    Options o = opt;
    o.mode = mode;
    o.max_steps = opt.max_steps.value_or(0);
    if (mode == Mode::Radial) {
        o.particles = std::numeric_limits<int>::max();
    }
    return o;
}

}  // namespace

int run_live(const Options& opt, std::uint64_t seed) {
    const int n = opt.size;
    const int max_steps = opt.max_steps.value_or(0);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(kWindowSize, kTopHeight + kWindowSize + kStatusHeight, "Cristal Simulation");
    SetTargetFPS(60);
    const Font font = load_ui_font();

    Mode mode = opt.mode;
    Simulation sim = create_simulation(live_options(opt, mode), seed);
    int step = 0;
    int steps_per_frame = 0;
    bool paused = false;

    auto restart = [&](Mode new_mode) {
        mode = new_mode;
        seed = std::random_device{}();
        sim = create_simulation(live_options(opt, mode), seed);
        step = 0;
        // Radial runs need many more steps until something visible happens.
        steps_per_frame = mode == Mode::Radial ? 128 : 2;
    };
    steps_per_frame = mode == Mode::Radial ? 128 : 2;

    std::vector<unsigned char> pixels = render_rgb(sim);
    Image image{pixels.data(), n, n, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8};
    Texture2D texture = LoadTextureFromImage(image);  // copies the pixels to the GPU
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);  // keep cells sharp when scaled up

    while (!WindowShouldClose()) {
        // --- input
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_RIGHT)) {
            steps_per_frame = std::min(steps_per_frame * 2, kMaxStepsPerFrame);
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_LEFT)) {
            steps_per_frame = std::max(steps_per_frame / 2, 1);
        }
        if (IsKeyPressed(KEY_R)) {
            restart(mode);
        }
        if (IsKeyPressed(KEY_M)) {
            restart(mode == Mode::Radial ? Mode::Uniform : Mode::Radial);
        }
        if (IsKeyPressed(KEY_S)) {
            const std::string file = "cristal_" + std::to_string(seed) + "_" + std::to_string(step) + ".png";
            TakeScreenshot(file.c_str());
        }

        // --- simulate
        const bool limit_reached = max_steps > 0 && step >= max_steps;
        const bool done = sim.finished() || limit_reached;
        if (!paused) {
            for (int k = 0; k < steps_per_frame && !sim.finished() && (max_steps == 0 || step < max_steps); k++) {
                sim.step();
                step++;
            }
        }
        pixels = render_rgb(sim);
        UpdateTexture(texture, pixels.data());

        // --- draw: square view centred between top bar and status bar
        const float view = static_cast<float>(
            std::min(GetScreenWidth(), GetScreenHeight() - kTopHeight - kStatusHeight));
        const Rectangle dest{(GetScreenWidth() - view) / 2.0f, static_cast<float>(kTopHeight), view, view};
        const Rectangle src{0.0f, 0.0f, static_cast<float>(n), static_cast<float>(n)};

        BeginDrawing();
        ClearBackground(kBackground);
        DrawTexturePro(texture, src, dest, Vector2{0, 0}, 0.0f, WHITE);

        // --- top bar: mode selection
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        DrawRectangle(0, 0, GetScreenWidth(), kTopHeight, kStatusBar);
        DrawTextEx(font, "MODE", {16.0f, (kTopHeight - kLabelSize) / 2.0f}, kLabelSize, 1.0f, kLabel);
        if (button(font, {64.0f, 8.0f, 100.0f, kTopHeight - 16.0f}, "Radial", mode == Mode::Radial)) {
            restart(Mode::Radial);
        }
        if (button(font, {172.0f, 8.0f, 100.0f, kTopHeight - 16.0f}, "Uniform", mode == Mode::Uniform)) {
            restart(Mode::Uniform);
        }
        if (done || paused) {
            const char* state = done ? (sim.hit_border() ? "DONE - reached the border" : "DONE") : "PAUSED";
            DrawTextEx(font, state, {292.0f, (kTopHeight - kButtonTextSize) / 2.0f}, kButtonTextSize, 1.0f, kAccent);
        }
        const std::string seed_text = "SEED " + std::to_string(seed);
        const Vector2 seed_size = MeasureTextEx(font, seed_text.c_str(), kLabelSize, 1.0f);
        DrawTextEx(font, seed_text.c_str(),
                   {GetScreenWidth() - 16.0f - seed_size.x, (kTopHeight - kLabelSize) / 2.0f}, kLabelSize, 1.0f,
                   kLabel);

        // --- status bar: fixed columns so the numbers never shift sideways
        const float bar_y = static_cast<float>(GetScreenHeight() - kStatusHeight);
        DrawRectangle(0, static_cast<int>(bar_y), GetScreenWidth(), kStatusHeight, kStatusBar);
        const float top = bar_y + 10.0f;
        draw_field(font, 120.0f, top, "STEP", group_digits(step), kValue);
        draw_field(font, 240.0f, top, "CRYSTAL", group_digits(static_cast<long long>(sim.crystal_count())), kValue);
        draw_field(font, 360.0f, top, "WALKERS", group_digits(static_cast<long long>(sim.walker_count())), kValue);
        draw_field(font, 470.0f, top, "STEPS/FRAME", group_digits(steps_per_frame), kValue);

        const char* hint1 = "Space pause   Up/Down speed   M mode";
        const char* hint2 = "R restart   S screenshot   Esc quit";
        const float right = static_cast<float>(GetScreenWidth()) - 16.0f;
        DrawTextEx(font, hint1, {right - MeasureTextEx(font, hint1, kLabelSize, 1.0f).x, top + 4.0f}, kLabelSize, 1.0f, kLabel);
        DrawTextEx(font, hint2, {right - MeasureTextEx(font, hint2, kLabelSize, 1.0f).x, top + 22.0f}, kLabelSize, 1.0f, kLabel);
        EndDrawing();
    }

    if (font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(font);
    }
    UnloadTexture(texture);
    CloseWindow();
    return 0;
}
