# Resource accounting and target verification

Updated: 2026-10-19. The selected display module is reported as ESP32-S3-WROOM-1-N4R8: 4 MB flash and 8 MB PSRAM. Verify the delivered revision and memory configuration before using these as target facts. Internal SRAM is a separate budget.

## Repeatable host reports

```sh
cmake --build build/lvgl --parallel 4
ctest --test-dir build/lvgl --output-on-failure
python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json
```

`host-metrics.json` records flush counts/pixels for 100 identical updates, LVGL object count, Dashboard wrapper size on the host ABI and host buffer sizes. Elapsed host microseconds are diagnostic and vary by load/build; they are not frame-rate or latency requirements.

`resource-audit.json` reports each generated asset's exact bitmap payload and allocated host object sections (code/constants/data/BSS as recognized by the platform size tool), excluding debug sections. Host metadata and alignment differ from ESP32. Host executable allocated sections include the simulated framebuffer in BSS and must never be labeled firmware flash usage. The report requires compiled asset objects; raw C file length is not used as a flash estimate. Source TTFs are converter inputs and are not included in the CMake runtime asset list.

After role-specific subsetting, font/icon bitmap payload totals 75,833 bytes and host asset sections total 85,796 bytes. Keep the 4 bpp rendering and inspect new glyph additions. The object tree remains 72 objects. The Dashboard wrapper is 840 bytes on the current Mac ABI (192 bytes of additional input tracking versus the previous stage); report the actual target size independently.

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

The shared feature profile is host-verified against LVGL 9.2.2. It is not a complete ESP-IDF configuration or a board driver. UART and settings/persistence are future feature implementations with explicit contracts; empty abstractions or unused bounded queues would provide no current optimization. See [optimization closure and remaining gates](optimization-plan.md).
