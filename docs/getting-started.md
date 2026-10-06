# Build and preview on your Mac

## What you are running

This is a native desktop application. Qt creates the window; QML describes its appearance; C++ supplies simulated bike values. There is no browser server or localhost URL.

The app opens at 800 × 480 logical pixels. On a Retina display, macOS uses extra physical pixels for sharp rendering. This is our first design canvas, not a confirmed final screen specification.

## Requirements

- Apple Command Line Tools (C++ compiler).
- Homebrew packages: `qt`, `cmake`, and `ninja`.
- Qt 6.5 or later; C++17.

## Launch the preview

Open Terminal and run:

```sh
cd /Users/codewavesandy/Desktop/learning/projects/r15-smart-cluster
bash scripts/preview.sh
```

The script configures the project, builds it, and opens the Mac app. The first build does more work; later builds reuse unchanged files. Keep the terminal output if an error occurs.

Click the app window before trying these keys:

| Key | Simulator action |
| --- | --- |
| Left / Right arrow | Decrease / increase speed by 5 km/h |
| Down / Up arrow | Decrease / increase RPM by 500 |
| 1–6 | Select gear |
| N or 0 | Select neutral |
| L / R | Toggle left / right indicator; cancels the other direction and hazards |
| Z | Toggle synchronized hazards; switching off clears both arrows |
| H | Toggle high-beam state |
| E | Toggle engine warning |
| [ / ] | Decrease / increase fuel by 5 percentage points |
| Tab | Switch Street / Track layout |
| Enter / Page Down / Page Up | Cycle lower information pages |
| S | Open settings |
| F1 | Open / close all keyboard controls |
| Q | Toggle QS enabled |
| P | Toggle app connection and sample phone battery |
| A / T / C | Toggle ABS / TCS / coolant warning |
| M / I | Toggle message / incoming call (requires app connection) |
| K / B | Start or record next lap / stop lap, in Track layout |
| Space | Reset bike signals (trips and laps reset in settings) |

Values are independent controls for exploring UI states; this is not a physics-based motorcycle simulation. Simulator limits (180 km/h, 13,000 rpm) are not claims about the R15's specifications. No key sends data to a real bike.

## Shift warning

At **10,500 RPM** the background turns red. At **11,400 RPM** it alternates red/normal every **300 ms**. Dropping below those thresholds returns to steady red or the normal theme. This replaces the on-screen shift-light dots. Arrow keys change RPM by 500, so the simulator reaches the flashing range at 11,500 when stepping upward from zero; automated checks cover the exact 11,400 boundary.

The thicker RPM bar blends green/yellow at 3,500–4,500 and yellow/red at 7,500–8,500 RPM. VVA still uses its separate above-7,500 rule.

## Settings and information

Press **S**, use **Up/Down** to select a row, and **Enter** to activate it. **Esc** returns to the preceding screen. Menu arrows do not change simulated speed or RPM.

Select **Time & date** to open the editor. **Left/Right** chooses a field, **Up/Down** adjusts it, and **Enter** advances to the next field or saves at the Save button. **Esc** cancels. This changes only the cluster clock. Mode, theme, units, and clock offset survive app restarts.

Street and Track are different display layouts. In Track, **K** starts timing; each subsequent K records the completed lap and begins the next. **B** stops and retains the current elapsed value; it does not record it as a completed lap. The next K starts a new lap. Settings can clear the lap session.

The reference details and remaining integration gaps are recorded in [OEM inventory](reference/oem-inventory.md). App connectivity, QS, warnings, battery, and coolant are simulated. Trips accumulate from simulated speed; no real odometer data is read or written. VVA appears above 7,500 RPM and disappears at or below it.

## Change something and see it

1. Quit the app with Command-Q.
2. Edit `ui/Main.qml`, for example the `accent` color.
3. Save and run `bash scripts/preview.sh` again.
4. The updated application opens.

This initial workflow rebuilds the app; it does not hot-reload. Closing only the window is not a reliable replacement for quitting the app before relaunching a changed build.

## Where things live

| File | Responsibility |
| --- | --- |
| `ui/Main.qml` | Window, dashboard layout, and temporary keyboard simulation bindings |
| `app/BikeState.h` | C++ values exposed to QML with change notifications |
| `app/main.cpp` | Starts Qt and supplies state, display preferences, and ride computer |
| `app/DisplaySettings.h` | Persistent display choices and validated clock/date editing |
| `app/RideComputer.h` | Simulated trip distance and monotonic lap timing |
| `CMakeLists.txt` | Describes the executable and QML module to the build system |
| `scripts/preview.sh` | Repeatable build-and-open command |
| `build/` | Generated files and app; excluded from Git |

Next we will extract the simulator controls into a dedicated source and define unavailable/stale data behavior. The current state object is a starting point, not the complete hardware contract.

## Build without opening a window

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Troubleshooting

- **Qt cannot be found:** ensure Homebrew's Qt installation completed, then rerun the configure command above.
- **Keys do nothing:** click inside the dashboard to make it the active window.
- **Old design remains visible:** quit the running app before rebuilding and reopening it.

## Checks run by the developer

```sh
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=offscreen build/r15-cluster.app/Contents/MacOS/r15-cluster --smoke-test
```

CTest checks simulator boundaries, VVA/phone behavior, keyboard-driven screens, date validation/save/cancel, persistent preferences, trips, and laps. UI tests render sample screens into `build/previews/` using an offscreen backend and isolated settings. The smoke check verifies QML loads and then exits; it does not replace visual or keyboard interaction testing.

For background, Qt's official [QML module guide](https://doc.qt.io/qt-6/qt6-port-to-qt-add-qml-module.html) explains the module/build approach used here.
