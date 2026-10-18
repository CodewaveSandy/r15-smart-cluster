# Preview and tune the UI on macOS

Updated: 2026-10-18. This is a native LVGL application with simulated observations, not a browser page or hardware firmware.

## 1. Open Terminal

```sh
cd /Users/codewavesandy/Desktop/learning/projects/r15-smart-cluster
```

## 2. Check the prerequisites

The current Mac already has the compiler, CMake, Ninja and SDL2. On another Mac, install Apple's Command Line Tools and the required Homebrew packages:

```sh
xcode-select --install
brew install cmake ninja sdl2
```

Only run the installation commands for missing tools. The first configure downloads the official LVGL 9.2.2 archive and verifies its pinned SHA-256. Subsequent builds reuse it. Fonts are already included as generated C assets; Node/npm is not needed for ordinary preview builds.

## 3. Build and launch

```sh
bash scripts/preview.sh
```

The script configures a Debug build under build/lvgl, compiles and opens an 800 x 480 native window. Boot artwork runs for 1.25 seconds before the dashboard. Click the window if keys do not respond. Terminal stays attached for diagnostics; quit with Esc or close the window.

## 4. Explore representative states

| Key | Action |
| --- | --- |
| Left / Right | Speed -/+ 1 km/h |
| Down / Up | RPM -/+ 100 |
| [ / ] | Fuel -/+ 1 percentage point |
| 0–6 | Neutral or gear |
| Tab | Street / Track |
| D | Midnight / Daylight |
| U | Metric / imperial speed and distance |
| Enter | ODO, battery or coolant information |
| L / R | Exclusive left / right simulated indicator |
| Z | Hazards |
| H | High beam |
| A / E / C | ABS / engine / coolant warning |
| X | Toggle unavailable observations |
| O | Simulated key-off / key-on |
| Space | Reset demo values |
| F1 | Print controls in Terminal |
| Esc | Quit |

Fuel: 10–20% yellow, below 10% red, blinking below 20%. RPM: steady shift alert from 10,500; flashing from 11,400. These are UI demonstration thresholds, not verified vehicle limits. Clock/date uses the Mac's local wall clock; deterministic renders use fixed time.

O simulates ignition; the Mac window stays open after shutdown so O can restart it. It does not cut power. A valid engine/coolant warning exposes instrumentation during boot/shutdown artwork. Real ignition, retained power and final critical-warning classification require hardware design/testing.

## 5. Change the design and rebuild

1. Quit the preview with Esc.
2. Edit the relevant source file and save.
3. Run bash scripts/preview.sh again.

There is no automatic hot reload yet. Ordinary source changes rebuild incrementally.

| File | What to tune |
| --- | --- |
| embedded/ui/Tokens.h | Theme palette, shared metrics and warning timing |
| embedded/ui/Dashboard.cpp | Dashboard geometry, labels, instrument composition and initial animation artwork |
| embedded/ui/InstrumentPresentation.h | Fuel/shift presentation rules, independent of LVGL |
| embedded/application/Lifecycle.h | Monotonic lifecycle; animationMs is currently 1250 |
| simulator/lvgl/main.cpp | Demo starting values, host keyboard mapping, clock and render scenarios |
| shared/model/ClusterState.h | Unit-qualified normalized observations and quality |
| scripts/generate-fonts.sh | Reproducible font sizes/glyph subsets |

Keep vehicle calculations and simulated mutations out of Dashboard.cpp. Colors do not change vehicle behavior. Use tests when changing thresholds/timing.

## Fonts

Bricolage subsets use lv_font_conv 1.5.3, 4-bit antialiasing and no compression. Large numeric fonts contain a limited glyph set; the 46 px animation/gear font also includes uppercase letters. Add required glyphs when changing text to avoid runtime missing-glyph warnings.

To regenerate after a font-size/glyph change (Node/npm required):

```sh
bash scripts/generate-fonts.sh
bash scripts/preview.sh
```

The converter is installed using tools/fonts/package-lock.json when absent. Generated font C files are deliberate runtime assets; ship their OFL license.

## Tests and screenshots

```sh
ctest --test-dir build/lvgl --output-on-failure
```

This runs portable model/lifecycle/presentation checks and renders 24 deterministic scenarios with SDL's dummy video driver. The render test rejects LVGL warnings and checks that low-fuel blinking reaches the pixels. BMP images are written to build/lvgl/previews and can be opened with macOS Preview.

To build and render without opening a window:

```sh
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews
```

Render coverage includes both layouts/themes, fuel blink phases, warnings, missing data, boot, shutdown, off and urgent-warning interruption. Review screenshots visually; tests alone do not establish design quality.

## Current limits

This first slice implements the dashboard foundation and simple progress-line boot/shutdown artwork. Several icons are simplified; final icon artwork, full settings/clock editing, functional laps/trips, persistence, automatic live reload, UART, display drivers and firmware flashing remain pending. Demo ODO is clearly labeled and never vehicle-owned persistence. A successful Mac preview is not ESP32 performance or bike verification.

If SDL2 is not found, confirm brew list sdl2 and pass its installation prefix through CMAKE_PREFIX_PATH when configuring manually. If the first dependency download fails, restore network access and rerun. Do not change board pins or flash parameters to solve host build issues.
