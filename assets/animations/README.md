# R15 Light Signature artwork

Current design: user-selected [concept 02](../../docs/reference/r15-light-signature-concept.png). Two mirrored blue light slashes are drawn procedurally in LVGL; a small solid-white R15 wordmark sits between them. Boot lasts three seconds; shutdown lasts two seconds. The dashboard is unchanged.

- [Source wordmark](source/r15-signature.png): white-on-black PNG generated with the built-in imagegen tool from the approved concept.
- [Accepted prompt](prompts.md): reference role and generation request.
- `lvgl/r15_signature.c`: 192 × 64 A8 mask, **12,288 bytes** before descriptor/alignment overhead.
- `manifest.json`: source/generated SHA-256 hashes and converter version.

The source PNG is a tool input, not a firmware resource. Conversion takes luminance as alpha: black becomes transparent and white becomes coverage, with antialiased edges. Black source padding is trimmed and the mark is fitted into a centered 160 × 40 region inside the 192 × 64 mask, preserving its aspect ratio. The renderer supplies the white color. This replaces the former 336 KiB glow textures; those assets and their renderer/timeline are removed after backup. There is no tuning-fork image, frame sequence, PNG decoder, oversized zoom or runtime vector parser.

Regenerate with Python and the tool-only Pillow version pinned in tools/animations:

```sh
python3 scripts/generate-animation-assets.py
```

Normal builds do not need Pillow. R15 artwork is a generated visual recreation, not verified official master artwork. See [lifecycle behavior](../../docs/power-lifecycle.md) and [resource accounting](../../docs/resource-budgets.md). Actual panel timing, transient drawing memory and retained shutdown power remain hardware verification gates.
