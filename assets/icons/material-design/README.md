# Dashboard icon sources

Source: [Pictogrammers Material Design Icons](https://pictogrammers.com/library/mdi/), official [@mdi/font 7.4.47 package](https://registry.npmjs.org/@mdi/font/-/font-7.4.47.tgz). The unmodified source font is bundled for reproducible offline conversion, not linked into the application.

The font and selected icons are licensed under Apache 2.0. Preserve [upstream license notice](LICENSE.txt), [Apache license](Apache-2.0.txt), and [NOTICE](NOTICE.txt) with distributed generated assets. No brand/logo glyphs are selected.

Pinned package version, archive/source-font hashes, original codepoints, selected names and raster sizes are recorded in [manifest.json](manifest.json). These are generic symbols; no OEM/regulatory conformity is claimed.

| Role | Upstream symbol |
| --- | --- |
| Left/right turn | arrow-left-bold / arrow-right-bold |
| High beam | car-light-high |
| Traction control | car-traction-control |
| ABS | car-brake-abs |
| Engine warning | engine |
| Coolant temperature | coolant-temperature |
| Message | message-text |
| Call | phone |
| Hazard | hazard-lights |
| Phone connection | cellphone |
| Fuel | gas-station |

Neutral stays a typographic N. QS/VVA keep their badges. The traction-control glyph uses a generic slipping-car silhouette; this does not verify Yamaha's signal or warning semantics.

Run bash scripts/generate-icons.sh to produce only these 12 glyphs as LVGL C assets. Converter: the existing locked lv_font_conv 1.5.3. Output is 4 bpp antialiased/uncompressed at 32 px, except ABS at 30 px to fit the ring cleanly inside a 32 px widget. No runtime SVG parser, full icon-font embedding, installed fonts or network access is required to render.
