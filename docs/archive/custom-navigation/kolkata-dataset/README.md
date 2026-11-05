# Bundled phase 1 Kolkata region

These are intentional licensed runtime assets, not temporary build exports. They provide the first central-Kolkata region to both phone and cluster without a hosted map backend.

- `kolkata.r15b`: exact v1 complete package (30,066 bytes), SHA256 `3443bb2dab6efe9a91840b0c8f375cfd970398c86c6fac0d23c93327fdd91685`.
- `kolkata.geojson`: matching integer-metre road geometry reprojected to WGS84 for MapLibre.
- Manifest, provenance and licence accompany the derived OSM data. Software and data licences are separate.

Regenerate with `python3 scripts/import-osm-map.py` from the preserved source snapshot, inspect the package/manifest/GeoJSON together, then deliberately update these runtime assets as one revision. Temporary downloads and preview outputs remain under ignored `build/maps/kolkata/`. No current/user location is included. No public Overpass calls occur in the phone app. Attribution is visible in Navigate and available in the licence dialog.

© OpenStreetMap contributors. ODbL1.0. https://www.openstreetmap.org/copyright
