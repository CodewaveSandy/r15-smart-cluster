# LVGL dashboard icon subsets

Generated runtime assets: cluster_icons_32.c (16 glyphs) and cluster_icons_30.c (ABS).

Sources, upstream license, notices and exact selection are in [material-design](../material-design/README.md). Run bash scripts/generate-icons.sh to regenerate with the locked font converter. Keep generated assets checked in; do not edit bitmap data by hand.

The UI uses typed Icon roles through embedded/ui/Icons.h, creates persistent glyph widgets, and changes text color/opacity for state. There are no hand-drawn substitute paths or runtime vector dependencies. Settings adds palette, Bluetooth, Wi-Fi and location glyphs from the same pinned source. Neutral tell-tale is an Inter N; QS/VVA remain text badges. Assets render at native size on both host and eventual panel.

Glyph presence and 32 px bounds are checked by typography-bounds; lvgl-render checks all-icons mode in both themes/layouts, along with normal, warning, unavailable and low-fuel states.
