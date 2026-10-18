# LVGL Bricolage runtime font assets

Generated from the bundled Bricolage Grotesque fonts; license and upstream hashes are in ../bricolage/README.md and ../bricolage/OFL.txt.

Reproduce with scripts/generate-fonts.sh from the repository root. Converter: lv_font_conv 1.5.3, pinned in tools/fonts/package-lock.json. Settings: LVGL C output, 4 bpp, uncompressed. No system fonts or runtime network requests.

| Asset | Size | Source/subset |
| --- | --- | --- |
| bricolage_small | 12 px | Regular, ASCII + degree/middle-dot/multiplication/em dash |
| bricolage_medium | 23 px | SemiBold, same text subset |
| bricolage_clock | 30 px | SemiBold, same text subset |
| bricolage_large | 46 px | Bold, digits/uppercase/selected punctuation/em dash |
| bricolage_track | 79 px | Bold, numeric/N/punctuation/em dash |
| bricolage_speed | 128 px | Bold, numeric/N/punctuation/em dash |

These generated C files are intentional distributable assets. Do not edit glyph data by hand. Target flash fit remains to be measured with the firmware build.
