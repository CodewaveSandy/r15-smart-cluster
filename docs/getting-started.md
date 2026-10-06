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
| L / R | Toggle left / right indicator |
| H | Toggle high-beam state |
| E | Toggle engine warning |
| [ / ] | Decrease / increase fuel by 5 percentage points |
| Space | Reset simulation |

Values are independent controls for exploring UI states; this is not a physics-based motorcycle simulation. Simulator limits (180 km/h, 12,000 rpm) are not claims about the R15's specifications. No key sends data to a real bike.

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
| `app/main.cpp` | Starts Qt and supplies BikeState to the window |
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

CTest checks simulator state boundaries, toggles, reset, and notifications. The smoke check verifies QML loads and then exits; it does not replace visual or keyboard interaction testing.

For background, Qt's official [QML module guide](https://doc.qt.io/qt-6/qt6-port-to-qt-add-qml-module.html) explains the module/build approach used here.
