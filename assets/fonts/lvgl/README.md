# LVGL runtime font assets

Inter Medium/SemiBold supplies labels, supporting values and headings. Barlow Semi Condensed SemiBold supplies large numerals, gear and clock. Both are bundled under SIL OFL 1.1; see [Inter provenance](../inter/README.md) and [Barlow provenance](../barlow-semi-condensed/README.md).

Reproduce with scripts/generate-fonts.sh. Converter: lv_font_conv 1.5.3, pinned in tools/fonts/package-lock.json. Settings: LVGL C output, 4 bpp, uncompressed. No system fonts or runtime network requests.

| Asset | Role | Size / weight |
| --- | --- | --- |
| inter_caption_12 | Tick labels and date | 12 px Medium |
| inter_label_14 | General labels/status | 14 px Medium |
| inter_value_16 | RPM title and value | 16 px Medium |
| inter_unit_18 | Speed unit | 18 px Medium |
| inter_heading_23 | Mode and supporting values | 23 px SemiBold |
| barlow_gear_72 | Gear | 72 px SemiBold |
| barlow_secondary_46 | Track speed | 46 px SemiBold |
| barlow_speed_128 | Street speed | 128 px SemiBold |
| barlow_track_79 | Track lap timer | 79 px SemiBold |
| barlow_clock_30 | Clock | 30 px SemiBold |

Text fonts include ASCII, degree, middle dot, multiplication sign and em dash. Large subsets are role-specific: gear uses 1–6/N; Street speed uses digits; Track lap uses digits/colon/full stop; clock uses digits/colon; secondary speed uses digits. Every subset includes the unavailable em dash. Generate required glyphs when adding text; missing-glyph warnings fail render tests.

Typography roles are centralized in embedded/ui/Typography.h. These C files are intentional runtime assets; do not edit glyph data manually. Flash fit must be measured in the eventual firmware build. Mac Retina interpolation improves enlarged preview presentation but does not increase the physical panel's resolution or font coverage.

Audit payload and compiled host object sections with `python3 scripts/audit-resources.py --build-dir build/lvgl --output build/lvgl/previews/resource-audit.json`. All font/icon bitmaps total 72,633 bytes after removing unused animation-title glyphs; this is not the total application image. See [resource accounting](../../../docs/resource-budgets.md).
