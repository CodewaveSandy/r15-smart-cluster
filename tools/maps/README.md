# Map development tools

These are data preparation, developer preview and paired-radio bench tools, not automated suites. No tool approves pairing, erases a bond, flashes a board or supplies synthetic GPS to physical firmware.

1. Generate all outputs from the cached central-Kolkata source:

   ```sh
   python3 scripts/import-osm-map.py
   ```

   `build/maps/kolkata/` contains the host `.r15map`, complete `.r15b`, `.manifest.json`, matching `.geojson`, provenance and licence. First download only: append `--download`. Do not use public Overpass as an app backend. The wire package is not a firmware asset and stays untracked.

2. Build the actual store/tunnel/Dashboard developer preview and inspect its captures:

   ```sh
   cmake -S . -B build/lvgl
   cmake --build build/lvgl
   SDL_VIDEODRIVER=dummy build/lvgl/simulator/lvgl/r15-map-firmware-preview.app/Contents/MacOS/r15-map-firmware-preview --package build/maps/kolkata/kolkata.r15b --render build/lvgl/map-firmware-previews
   ```

   Every frame is explicitly BENCH DEMO. The optional simulated position in this host-only executable is not live location and is never sent to a board. Output includes accepted byte offsets, state/errors and package SHA; manually compare to the manifest. Captures cover both layouts/themes. No radio/board acceptance is implied.

3. Build/package the two boards independently using existing guards:

   ```sh
   bash scripts/flash-vehicle.sh --build-only
   bash scripts/flash-display.sh --build-only
   ```

   Physical flashing uses the existing documented commands only after connecting the identified boards; do not bypass chip, backup or partition guards. Then record the [phase 1 acceptance](../../docs/map-phase-1.md#8-phase-1-acceptance-and-evidence).

4. Optional paired BLE bench transfer, before Android map implementation:

   ```sh
   python3 -m venv build/map-tools
   build/map-tools/bin/python -m pip install bleak==1.1.1
   build/map-tools/bin/python tools/maps/send-map.py --device YOUR_OS_BLE_IDENTIFIER --package build/maps/kolkata/kolkata.r15b
   ```

   [Bleak 1.1.1](https://pypi.org/project/bleak/1.1.1/) is the pinned optional desktop dependency; it is not installed by the importer or firmware. Use the explicit OS identifier of the selected `R15 Cluster`; discovery names alone do not authenticate. CoreBluetooth does not expose negotiated MTU: default commands fit MTU96; `--mtu 247` is only for a verified actual limit. Firmware rejects insufficient MTU. The tool serializes response writes, subscribes/reads status, uses display ACK offsets and checks ACTIVE, with bounded retries. It sends no LOCATION and closes on failure; retry starts a new connection/BEGIN, with complete-content SHA reuse supported.

   **One saved peer remains the security policy.** The bench Mac must already be that authorized peer through the normal visible cluster/OS numeric comparison. An existing Android bond does not authorize the Mac. Any deliberate Forget/re-pair/restore is a manual user action; this tool never performs it. Obtain the identifier using the OS's BLE tooling; do not paste identifiers or location packets into public logs. Stop the phone client before a Mac transfer. If retaining the phone bond is preferred, defer physical map transfer until the Android integration exists. The tool does not keep c012 phone metadata alive and is not a substitute for the later coexistence/pocket acceptance run.

The byte authority is [mobile protocol](../../mobile/docs/protocol.md#phase-1-map-extension--implementation-specification); [normative vectors](../../shared/protocol/map-phase-1-vectors.txt) are language-neutral data for manual review/future codecs. No automated suites have been added or resumed.
