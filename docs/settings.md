# Settings and instrument transitions

Updated: 2026-10-19. First host slice implemented; submenus and persistence pending.

## Visual direction

The user selected the category-grid concept in [settings-concept.png](reference/settings-concept.png). The image is an AI-generated design reference, not evidence of connected radios. Actual host screens retain all dashboard tell-tales below the persistent gear/speed/clock strip. The four categories are Appearance, Bluetooth, Wi-Fi and Navigation, with a clear theme-accent focus outline. Both Midnight and Daylight are implemented.

## First slice

SettingsHome owns a persistent four-tile tree. SettingsNavigation reduces semantic actions without LVGL, SDL, storage or radio dependencies. The host maps S to Open, Right/Down to next tile and Left/Up to previous tile, I to sequential focus, and Backspace/Esc to Back. The category focus is retained for the current process. Enter/select does not yet open submenus. Existing dashboard actions are suppressed while Settings is open, apart from host theme/unit inspection shortcuts.

A valid speed observation from 0 up to (but excluding) 20 km/h, Running lifecycle and no valid urgent warning are required to open Settings. Speed at/above 20 km/h, unavailable/stale speed, urgent warning or power transition close it. The renderer independently enforces the same gate so a stale view option cannot hide instruments. Physical input detection and the vehicle command channel are unimplemented.

The same persistent speed label travels between the dashboard and top strip over 280 ms, easing position and scaling the Street numerals to approximately the existing 46 px secondary size. Track speed retains its secondary size and moves to the same strip. The unit follows the visible glyph edge. Reversing the request preserves the current transition position. Normal dashboard geometry is restored exactly. Gear and tell-tales remain live; current/best lap return with Track instrumentation.

A monotonic portable transition runs under the existing LVGL owner. No widget timer, task or per-update application allocation is added. The instrument group is hidden without a full-screen alpha layer; category backgrounds/borders and bounded labels fade separately. LVGL may allocate bounded intermediate layers for text transforms. Actual target heap, render/scanout timing and DMA/PSRAM behavior remain hardware gates.

## Next slices

1. Appearance: theme/design selection, preview, Apply and Cancel. The current design is the only offered design. Confirmed preferences survive restart via a versioned validated host file adapter and later display-owned NVS adapter; cancelled previews do not persist. Storage work belongs outside LVGL rendering. Persistence is not implemented in this first slice.
2. Bluetooth: supported pairing/discovery/confirmation, paired devices, Disconnect and Forget. UI consumes confirmed vehicle-radio status and issues typed requests; no optimistic connection state.
3. Wi-Fi: network identity, connection progress, credentials, Disconnect and Forget. Whether the phone is hotspot, peer or another endpoint remains unresolved.
4. Navigation: configuration/status first. Full Google Maps/projection remains a separate feasibility requirement; GPS position alone is not a map renderer.

Submenus share Next/Previous/Select/Back semantics. Touch may be added as another adapter; physical switches must remain sufficient for navigation, with text entry evaluated separately. The radio tiles currently say Not configured/Setup required rather than claiming an implemented service.

## Verification

Portable checks cover speed-boundary/quality/lifecycle/warning gates, focus wrapping, reversals and immediate interruption. Render scenarios cover entry, settled focus, exit, urgent warnings and stale speed for both layouts/themes, exact dashboard restoration, and zero idle redraws on Settings and dashboard. Host rendering is not board or bike verification.

The 20 km/h access threshold is user-selected UI behavior, not a validated safe riding speed. The gate always uses normalized km/h even when display units are imperial. Future pairing/text-entry restrictions require their own review.
