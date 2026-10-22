# Resource accounting and target verification

Updated: 2026-10-19. The selected display module is reported as ESP32-S3-WROOM-1-N4R8: 4 MB flash and 8 MB PSRAM. Delivered PCB V3.0 and esptool-reported 4 MB flash/8 MB PSRAM were confirmed on 2026-10-22; R15 runtime allocation/performance still need panel measurements. Internal SRAM is a separate budget.

## Repeatable host reports

```sh
cmake --build build/lvgl --parallel 4
ctest --test-dir build/lvgl --output-on-failure
python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json
```

`host-metrics.json` records flush counts/pixels for 100 identical updates, LVGL object count, Dashboard wrapper size on the host ABI and host buffer sizes. Elapsed host microseconds are diagnostic and vary by load/build; they are not frame-rate or latency requirements.

`resource-audit.json` reports each generated asset's exact bitmap payload and allocated host object sections (code/constants/data/BSS as recognized by the platform size tool), excluding debug sections. Host metadata and alignment differ from ESP32. Host executable allocated sections include the simulated framebuffer in BSS and must never be labeled firmware flash usage. The report requires compiled asset objects; raw C file length is not used as a flash estimate. Source TTFs are converter inputs and are not included in the CMake runtime asset list.

Current font/icon bitmap payload is 90,564 bytes with the angular numeral subsets. The R15 Light Signature adds one 192 × 64 A8 mask (12,288 bytes), giving **102,852 payload bytes** and **113,130 compiled host asset section bytes** in total. The source PNG is not linked into firmware.

The object tree now contains 138 objects, including Settings home, Appearance and Phone status/action rows and the instrument visibility group. The Dashboard wrapper is 1,648 bytes on the current Mac ABI, including input tracking, animation state and 256 bytes of persistent readout text. Host buffers remain 768,000 bytes for the simulated RGB565 framebuffer and 64,000 bytes for the partial draw buffer. Measure actual target sizes independently.

## Buffer arithmetic, not an allocation prescription

| RGB565 allocation | Bytes | KiB |
| --- | ---: | ---: |
| One 800 × 480 framebuffer | 768,000 | 750 |
| Two full framebuffers | 1,536,000 | 1,500 |
| One 800 × 40 draw buffer | 64,000 | 62.5 |
| Two such draw buffers | 128,000 | 125 |

Do not sum every option and allocate them all. Scanout framebuffers, LVGL draw buffers and driver bounce buffers have different ownership. A compatible direct mode may share a framebuffer role; partial mode may need copies. Validate flush completion, VSYNC, cache/DMA alignment and memory capabilities using the exact board port. PSRAM capacity alone does not establish bandwidth or internal DMA/stack headroom.

## Required target report before claiming firmware readiness

| Category | Evidence |
| --- | --- |
| Flash | Bootloader/partition table, actual application image and section sizes, fonts/assets, NVS and required margin; OTA only if the selected slots and real images fit |
| Internal SRAM | Static allocation, DMA/bounce buffers, all task stacks, queues and driver allocations; minimum free and largest block under load |
| PSRAM | Framebuffer/draw/cache allocations, minimum free/largest block and fragmentation through repeated navigation/theme changes |
| Rendering | Render/flush timings, dirty area, VSYNC/tearing/color ordering and visible warning latency under representative peak work |
| Communication | Parser bounds, queue/mailbox high-water marks, overflow/error counters, reboot/link-loss/source-staleness behavior |
| Persistence/power | Write latency and rendering impact, power-loss recovery, write rate/wear policy, retained shutdown power and rapid cycling |

Record build revisions/toolchain, configuration, measurement method and load with every report. Use a reviewed headroom budget based on measured drivers/tasks; do not present unused PSRAM as interchangeable with internal SRAM. Allocator choice, core affinity, buffer placement and task priorities remain target decisions.

## Scope boundary

The shared feature profile is host-verified against LVGL 9.2.2. It is not a complete ESP-IDF configuration or a board driver. Host settings/persistence and simulated radio menus are implemented. UART, real radio services and firmware NVS require reviewed contracts and platform adapters; see [implementation gates](implementation-plan.md). Avoid unused abstractions or queues.

## Light Signature rendering cost

The renderer uses a constant number of line/triangle draws and one small A8 wordmark at native or smaller scale. LV_USE_IMAGE remains enabled; no PNG decoder, vector parser, full-screen texture or oversized image zoom is needed. Bounded application work does not prove a target frame rate: measure LVGL transient draw memory, stack/heap headroom, render/flush timing and retained power on the actual board. Host captures establish appearance/timing logic only.

## Readout and desktop presentation updates

Eight changing instrument labels (speed, current/best lap or distance, RPM, gear, phone battery, ride information and fuel) use component-owned 32-byte buffers with LVGL static text binding. After initial binding, these text replacements do not allocate label text storage. This does not claim all LVGL layout/render work is allocation-free. Buffers outlive their labels and are never moved. A host check exercises repeated length/quality changes and stable buffer addresses.

The SDL preview uploads its framebuffer only after an LVGL flush, and presents only after an upload or window repaint event. Exposure, resize and restore reuse the texture; renderer device reset recreates it and forces an upload. The render suite checks initial/changed uploads, 100 idle calls, and repaint without upload using a software SDL renderer. The event/UI service loop continues at its existing cadence. Target rendering and hardware memory measurements remain pending.

## First native display build (2026-10-22)

ESP-IDF 5.5.4, LVGL 9.2.2, Xtensa GCC 14.2.0, C++17. The R15 build and `flash-display.sh --build-only` pass. The packaged bootloader/table/application image is 669,472 bytes; application binary 603,936 bytes, within the 3,145,728-byte app partition. Section reporting shows 16,872 bytes static data/BSS (excludes runtime allocations and stacks). These are build results, not a hardware heap budget or measured frame rate. Runtime diagnostics, PSRAM buffers, two-VSYNC flush behavior and artwork timing still require the user's first R15 flash and panel inspection. The working vendor example is the only panel baseline currently tested.
