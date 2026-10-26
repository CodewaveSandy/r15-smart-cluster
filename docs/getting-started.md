# Preview and tune the UI on macOS

Updated: 2026-10-19. This is a native LVGL application with simulated observations, not a browser page or hardware firmware.

## 1. Open Terminal

```sh
cd /Users/codewavesandy/Desktop/learning/projects/r15-smart-cluster
```

## 2. Check the prerequisites

Install Apple's Command Line Tools and the required Homebrew packages:

```sh
xcode-select --install
brew install cmake ninja sdl2
```

Only run the installation commands for missing tools. The first configure downloads the official LVGL 9.2.2 archive and verifies its pinned SHA-256. Subsequent builds reuse it. Fonts are already included as generated C assets; Node/npm is not needed for ordinary preview builds.

## 3. Build and launch

```sh
bash scripts/preview.sh
```

The script configures a Debug build under build/lvgl, compiles and opens an 800 x 480 native window. The R15 Light Signature (3 s boot / 2 s shutdown) reveals the dashboard during its last 450 ms. Click the window if keys do not respond. Terminal stays attached for diagnostics; quit with Esc or close the window.

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
| Enter | Street only: ODO / Trip 1 / Trip 2; Track keeps BEST LAP fixed |
| I | Info list: coolant / Track mode / real-time fuel / average fuel / average speed / TCS / unidentified |
| T | Toggle selected TCS (simulator only) or Track mode |
| L / R | Exclusive left / right simulated indicator |
| Z | Hazards |
| H | High beam |
| A / E / C | ABS / engine / coolant warning |
| X | Toggle unavailable observations |
| O | Simulated key-off / key-on |
| Space | Reset demo values |
| F1 | Print controls in Terminal |
| P / S | Stationary neutral fixture / open Settings |
| F3 / F4 | Fail next Bluetooth request / toggle service availability |
| F5 / F6 | Fail next Wi-Fi request / toggle service availability |
| F2 | Illuminate all icons steadily; press again to restore normal status |
| Backspace / Esc | Back in Settings; Esc quits outside Settings |

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
| embedded/ui/components/ | Dashboard instruments, Settings/Appearance/radio panels and lifecycle artwork |
| embedded/presentation/ | Bounded instrument/ride-info formatting, quality decisions and fuel/shift policy |
| embedded/application/DisplayOptions.h and DashboardInput.h | Portable display-local preferences, pages and semantic actions |
| simulator/lvgl/DesktopInput.h | Host key translation into semantic actions |
| embedded/application/Lifecycle.h | Monotonic lifecycle; bootMs=3000 and shutdownMs=2000 |
| simulator/lvgl/main.cpp | Demo starting values, host keyboard mapping, clock and render scenarios |
| shared/model/ClusterState.h | Unit-qualified normalized observations and quality |
| scripts/generate-fonts.sh | Reproducible font sizes/glyph subsets |

Keep vehicle calculations and simulated mutations out of Dashboard.cpp and the UI components. Colors do not change vehicle behavior. Use tests when changing thresholds/timing.

## Fonts

Inter/Barlow/Chakra Petch subsets use lv_font_conv 1.5.3, 4-bit antialiasing and no compression. Edit embedded/ui/Typography.h for role mapping and scripts/generate-fonts.sh for font sizes. RPM caption/suffix are 16 px Medium and RPM digits are 16 px SemiBold; general labels are 14 px. The Mac preview uses linear filtering for Retina enlargement. Large numeric fonts contain a limited glyph set; the 46 px secondary-speed font contains only digits and the unavailable dash. Add required glyphs when changing text to avoid runtime missing-glyph warnings.

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

This runs eleven suites: Phone menu/IPC, portable model/lifecycle, Light Signature timeline and presentation/input checks, desktop key mappings, Bluetooth and Wi-Fi state/requests, preference storage/restart checks, typography bounds/glyph coverage, and deterministic dashboard/Phone render scenarios with SDL's dummy video driver. The render test rejects LVGL warnings and checks that low-fuel blinking reaches the pixels. BMP images are written to build/lvgl/previews and can be opened with macOS Preview. That directory also receives host-metrics.json with unchanged-update flush counts, object count, host buffer sizes and diagnostic host timing; these are not ESP32 measurements. Stable-state tests reject redundant flushes.

To build and render without opening a window:

```sh
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --render build/lvgl/previews
```

Render coverage includes both layouts/themes, fuel blink phases, warnings, missing data, boot, shutdown, off and urgent-warning interruption. Review screenshots visually; tests alone do not establish design quality.

## Current limits

For the opt-in discrete-input hardware experiment, use `bash scripts/preview.sh --vehicle-serial /dev/cu.YOUR_VEHICLE_PORT`. See the [vehicle USB input test](../firmware/vehicle/README.md) for wiring, firmware and per-channel pin/polarity configuration. Gear N–6, left/right and high beam come from configured USB inputs; other vehicle readings remain unavailable. The ordinary preview remains simulated.

This host slice implements the dashboard foundation and R15 Light Signature (3 s boot / 2 s shutdown). Icons use bundled automotive glyphs; Navigation/clock editing, functional laps/trips, firmware NVS persistence, automatic live reload, UART, display drivers and firmware flashing remain pending. Appearance and host saved preferences are implemented; Phone is a unified page; real discovery/session behavior requires the opt-in connection preview. The window title and terminal identify all observations as simulated. ODO/trip values are fixed host samples and never vehicle-owned persistence. A successful Mac preview is not ESP32 performance or bike verification.

If SDL2 is not found, confirm brew list sdl2 and pass its installation prefix through CMAKE_PREFIX_PATH when configuring manually. If the first dependency download fails, restore network access and rerun. Do not change board pins or flash parameters to solve host build issues.

Instrument layouts, information-page order and visual thresholds are defined in the [UI style guide](lvgl-style-guide.md). Implementation boundaries and pending work are tracked in the [implementation plan](implementation-plan.md).

## Icon assets

Automotive/status symbols come from the pinned Pictogrammers MDI 7.4.47 source font. Only 16 glyphs are compiled into LVGL assets; original source/licenses and hashes are bundled under assets/icons/material-design. Typed roles and widget creation live in embedded/ui/Icons.h and Icons.cpp. Regenerate after changing the selection with bash scripts/generate-icons.sh. Press F2 to inspect all symbols in both themes/layouts. See [icon provenance](../assets/icons/material-design/README.md).

## Resource audit and feature profile

Run `python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json` after building to inspect bitmap payload and host asset sections. See [resource budgets](resource-budgets.md) for limitations and required target measurements. The host includes embedded/ui/LvglFeatures.h, which enables only current dashboard widgets and reuses bundled Inter as the default font. Enable additional LVGL features explicitly when actual screens require them, then rebuild and verify. Numeric text now formats on changed input, and covered boot/shutdown instrumentation resumes from the latest snapshot. Settings reuses arrows for sequential focus instead of telemetry adjustment.

## R15 Light Signature preview

Restart with `bash scripts/preview.sh` to see the three-second boot. Press O for the two-second shutdown; press O again to boot. A valid engine/coolant warning bypasses artwork immediately; press E/C to clear a simulated warning before reviewing the complete sequence. Key-on during shutdown cancels it. The last shutdown interval and off state are completely black; F1 prints controls in Terminal.

For deterministic animation captures without a window:

```sh
SDL_VIDEODRIVER=dummy bash scripts/preview.sh --animate build/lvgl/animation-preview
```

This saves the standard dashboard/Phone scenarios plus 76 boot and 51 shutdown motion frames at 40 ms intervals. Source/generated assets and prompts are in assets/animations; Pillow 12.3.0 is needed only for regeneration, not normal builds. See [lifecycle timeline](power-lifecycle.md). The artwork uses one 12 KiB A8 wordmark and procedural blue light slashes. Actual panel performance remains unverified. To export GIF/contact-sheet previews after --animate, run `python3 scripts/export-animation-previews.py build/lvgl/animation-preview` with Pillow installed.

Fuel preview: `[` / `]` change the simulated reading by 1%. At 56% the fifth piece is partly filled; at 6% the first piece is roughly half filled. `RESERVE` appears at/below 16% for a valid reading, matching the user-supplied approximate reserve ratio. These are host samples, not calibrated fuel-volume measurements.

## Settings and Appearance

Restart with `bash scripts/preview.sh`. After boot, press **P** to set the host demo to stationary/neutral, then **S** to open Settings. **Right/Down** advance through Appearance → Phone → Navigation, wrapping around; **Left/Up** go backward. **I** also cycles forward. **Backspace** or **Esc** returns with the speed transition (Esc closes the app when outside Settings). **D** still previews the other theme. **Enter** opens Appearance when its tile is focused; Phone also opens with Enter; Navigation remains pending. Settings closes immediately if speed becomes stale or reaches 20 km/h, a valid urgent warning appears or the lifecycle leaves Running. See [Settings design](settings.md).

In Appearance, use arrows to focus Midnight/Daylight and **Enter** to preview it. Continue through Instrument, Cancel and Apply; Enter activates the focused choice. Apply saves the confirmed theme/design for the next Mac preview launch. Cancel/Back restores the previous theme. The save file is `~/Library/Application Support/R15 Smart Cluster/preferences.bin`; use `R15_SETTINGS_PATH` for an isolated override. **D** remains a temporary host preview shortcut and does not save automatically. Only theme/design are persistent in this slice.

## Companion connection preview

An opt-in real Wi-Fi connection between the Android companion and Mac preview is available through `bash scripts/preview.sh --phone-bridge` and `bash scripts/preview-mobile.sh --mac-preview`. See the [connection guide](../mobile/docs/mac-preview-connection.md) for BLE discovery, first-time cluster approval, saved reconnect and development-only limits. USB is used to install/debug the app, not to carry connection traffic. Vehicle readings stay simulated and native BLE discovery/cluster approval are implemented. ESP32 Wi-Fi provisioning remains pending.
