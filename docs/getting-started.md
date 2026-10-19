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

The script configures a Debug build under build/lvgl, compiles and opens an 800 x 480 native window. The two-second R15 Light Signature reveals the dashboard during its last 300 ms. Click the window if keys do not respond. Terminal stays attached for diagnostics; quit with Esc or close the window.

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
| Enter | Street only: ODO / Trip A / Trip B / Trip F; Track keeps BEST LAP fixed |
| I | Info list: coolant / battery voltage / TCS / Track mode |
| T | Toggle selected TCS (simulator only) or Track mode |
| L / R | Exclusive left / right simulated indicator |
| Z | Hazards |
| H | High beam |
| A / E / C | ABS / engine / coolant warning |
| X | Toggle unavailable observations |
| O | Simulated key-off / key-on |
| Space | Reset demo values |
| F1 | Print controls in Terminal |
| F2 | Illuminate all icons steadily; press again to restore normal status |
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
| embedded/ui/Typography.h | Central font roles for labels, values and instruments |
| embedded/ui/Dashboard.cpp | Component composition and shift-background theme application |
| embedded/ui/Geometry.h | Named component positions and bounds, based on shared Metrics |
| embedded/ui/components/ | Header, tell-tales, tachometer, readouts, info, fuel and lifecycle artwork |
| embedded/presentation/ | Bounded instrument/ride-info formatting, quality decisions and fuel/shift policy |
| embedded/application/DisplayOptions.h and DashboardInput.h | Portable display-local preferences, pages and semantic actions |
| simulator/lvgl/DesktopInput.h | Host key translation into semantic actions |
| embedded/application/Lifecycle.h | Monotonic lifecycle; animationMs is currently 2000 |
| simulator/lvgl/main.cpp | Demo starting values, host keyboard mapping, clock and render scenarios |
| shared/model/ClusterState.h | Unit-qualified normalized observations and quality |
| scripts/generate-fonts.sh | Reproducible font sizes/glyph subsets |

Keep vehicle calculations and simulated mutations out of Dashboard.cpp and the UI components. Colors do not change vehicle behavior. Use tests when changing thresholds/timing.

## Fonts

Inter/Barlow subsets use lv_font_conv 1.5.3, 4-bit antialiasing and no compression. Edit embedded/ui/Typography.h for role mapping and scripts/generate-fonts.sh for font sizes. RPM text is 16 px; general labels are 14 px. The Mac preview uses linear filtering for Retina enlargement. Large numeric fonts contain a limited glyph set; the 46 px secondary-speed font contains only digits and the unavailable dash. Add required glyphs when changing text to avoid runtime missing-glyph warnings.

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

This runs six suites: portable model/lifecycle, Light Signature timeline and presentation/input checks, desktop key mappings, typography bounds/glyph coverage, and 86 deterministic render scenarios with SDL's dummy video driver. The render test rejects LVGL warnings and checks that low-fuel blinking reaches the pixels. BMP images are written to build/lvgl/previews and can be opened with macOS Preview. That directory also receives host-metrics.json with unchanged-update flush counts, object count, host buffer sizes and diagnostic host timing; these are not ESP32 measurements. Stable-state tests reject redundant flushes.

To build and render without opening a window:

```sh
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews
```

Render coverage includes both layouts/themes, fuel blink phases, warnings, missing data, boot, shutdown, off and urgent-warning interruption. Review screenshots visually; tests alone do not establish design quality.

## Current limits

This host slice implements the dashboard foundation and two-second R15 Light Signature. Icons use bundled automotive glyphs; full settings/clock editing, functional laps/trips, persistence, automatic live reload, UART, display drivers and firmware flashing remain pending. The window title and terminal identify all observations as simulated. ODO/trip values are fixed host samples and never vehicle-owned persistence. A successful Mac preview is not ESP32 performance or bike verification.

If SDL2 is not found, confirm brew list sdl2 and pass its installation prefix through CMAKE_PREFIX_PATH when configuring manually. If the first dependency download fails, restore network access and rerun. Do not change board pins or flash parameters to solve host build issues.

The dashboard has no bottom help/status bar. F1 prints keyboard help in Terminal. Missing data appears as dashes and a DATA UNAVAILABLE header subtitle; F2 shows ICON TEST there. Trip/info selections are independent. TCS toggling changes a host sample only and sends no vehicle command. Track toggling changes the display layout.

Track mode prioritizes the large centered CURRENT LAP readout; speed moves to the smaller right-hand position. The preview supplies a fixed 01:24.38 lap sample for design review. Missing lap data shows dashes; lap start/stop/record functionality remains pending. Street mode retains centered speed.

Track shows BEST LAP in the lower-left panel instead of ODO/trips, with no distance unit. The host sample is 01:18.42; real record calculation/persistence is pending. Enter leaves that panel fixed and preserves the Street trip selection. Both layouts use the same vertical section heights.

## Icon assets

Automotive/status symbols come from the pinned Pictogrammers MDI 7.4.47 source font. Only 12 glyphs are compiled into LVGL assets; original source/licenses and hashes are bundled under assets/icons/material-design. Typed roles and widget creation live in embedded/ui/Icons.h and Icons.cpp. Regenerate after changing the selection with bash scripts/generate-icons.sh. Press F2 to inspect all symbols in both themes/layouts. See [icon provenance](../assets/icons/material-design/README.md).

## Resource audit and feature profile

Run `python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json` after building to inspect bitmap payload and host asset sections. See [resource budgets](resource-budgets.md) for limitations and required target measurements. The host includes embedded/ui/LvglFeatures.h, which enables only current dashboard widgets and reuses bundled Inter as the default font. Enable additional LVGL features explicitly when actual screens require them, then rebuild and verify. Numeric text now formats on changed input, and covered boot/shutdown instrumentation resumes from the latest snapshot. All existing keyboard controls remain unchanged.

## R15 Light Signature preview

Restart with `bash scripts/preview.sh` to see the two-second boot. Press O for the two-second shutdown; press O again to boot. A valid engine/coolant warning bypasses artwork immediately; press E/C to clear a simulated warning before reviewing the complete sequence. Key-on during shutdown cancels it. The last shutdown interval and off state are completely black; F1 prints controls in Terminal.

For deterministic animation captures without a window:

```sh
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --animate build/lvgl/animation-preview
```

This saves the 86 standard scenarios plus 51 motion frames per animation at 40 ms intervals. Source/generated assets and prompts are in assets/animations; Pillow 12.3.0 is needed only for regeneration, not normal builds. See [lifecycle timeline](power-lifecycle.md). The artwork uses one 12 KiB A8 wordmark and procedural blue light slashes. Actual panel performance remains unverified. To export GIF/contact-sheet previews after --animate, run `python3 scripts/export-animation-previews.py build/lvgl/animation-preview` with Pillow installed.
