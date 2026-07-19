# Wireless Corne ZMK Config — Claude Notes

## Project

Boardsource Wireless SMT Corne split keyboard running ZMK firmware.
- Left half = central (BLE HID host, USB, ZMK Studio)
- Right half = peripheral (BLE split, no USB)
- Displays: nice!view (68×160 Sharp Memory LCD) on both halves
- Board: `blecorne_left` / `blecorne_right` (nRF52840, Adafruit UF2 bootloader)
- CI: GitHub Actions → `zmkfirmware/zmk` build-user-config workflow (reads `build.yaml` only)

## Display Preview

**Always read and update `docs/display_preview.html` when working on display widgets.**

- Self-contained HTML artifact (JetBrains Mono embedded as base64 `@font-face`)
- Shows all left/right display states: BT status, battery, modifiers, layer circles, WiFi glyph
- Publish to Artifact at `https://claude.ai/code/artifact/8b062b93-4ffb-4774-9e6f-c256a5bd9e2a`
- Font files for rebuilding: JBMono 400/700 TTF — look in session scratchpad or re-download from JetBrains

### Mockup rendering rules
- All glyphs rendered as pixel-art SVG rects (NOT smooth curves) matching the actual L8 bitmap
- Use plain string concatenation in JS — no template literals (backtick regex injection risk)
- WiFi glyph: `wifiSvg(connected)` — 2× scale pixel-art, 28×28 viewBox in 14×14 CSS SVG
- Alt glyph: `altSVG()` — 2× scale pixel-art, `fill="currentColor"` for active/inactive states
- BT glyph: `btSvg(vis)` — inline SVG with opacity for connected/disconnected

## Display Architecture

### Canvas layout (nice!view 68×160, rotated 270° CW before display)

| Canvas | Driver x offset | Physical region | Content |
|--------|----------------|-----------------|---------|
| `canvas_bot` | −44 | Top 24 px strip | Status (BT, battery, profile / WiFi glyph) |
| `canvas_mid` | +24 | Middle 68 px | Modifier glyphs |
| `canvas_top` | +92 (TOP_RIGHT) | Bottom 68 px | Layer circles + name (central) / blank (peripheral) |

- Canvas is 68×68 px, L8 format (`0x00` = ink, `0xFF` = background)
- `canvas_bot` only shows rows 0..23 (24 px); glyphs placed at `y=13` show rows 0..10
- After drawing, call `rotate_canvas()` (270° CW via `lv_draw_sw_rotate`)
- `CONFIG_NICE_VIEW_WIDGET_INVERTED=y` → `LVGL_BACKGROUND=black`, `LVGL_FOREGROUND=white`

### Key source files

```
config/
  blecorne.conf               — Kconfig (display, BLE, fonts, inverted)
  blecorne.keymap             — Keymap (layers 0-8, combos, HRMs)
  boot_led.c                  — Boot flash: blinks blue_led (P1.09) 5× at SYS_INIT APPLICATION 5
  CMakeLists.txt              — Build sources (boot_led + display widgets)
  custom_status_screen.c      — ZMK display entry point
  widgets/
    util.h / util.c           — draw_battery, draw_glyph, draw_circle, rotate_canvas
    glyphs.h / glyphs.c       — L8 bitmap glyphs: shift, ctrl, alt, win, cmd, opt, bt, wifi
    blecorne_central.c        — Left half widget (status + mods + layer circles)
    blecorne_peripheral.c     — Right half widget (status + mods)
  split/
    modifier_sync_central.c   — BLE GATT client: writes R-mod nibble to peripheral
    modifier_sync_peripheral.c — BLE GATT server: receives R-mods, calls update_mods
boards/boardsource/blecorne/
  blecorne.dtsi               — Shared DTS: blue_led P1.09, ext_power P0.31, vbatt ADC
  blecorne_left.dts / _right.dts — Per-half kscan matrices
```

### Glyphs (`config/widgets/glyphs.h`)

All 14×14 L8 bitmaps. Active = `LV_OPA_COVER`, inactive = `LV_OPA_40` (both in `LVGL_FOREGROUND`).

| Symbol | Variable | Notes |
|--------|----------|-------|
| ⇧ Shift | `glyph_sft` | |
| ⌃ Ctrl | `glyph_ctrl` | Shared Win+Mac |
| ⎇ Alt | `glyph_alt` | Upward arrow + base bar |
| ⊞ Win | `glyph_win` | 4-pane grid |
| ⌘ Cmd | `glyph_cmd` | |
| ⌥ Opt | `glyph_opt` | |
| ᛒ BT | `glyph_bt` | Hagall-rune style |
| 📶 WiFi | `glyph_wifi` | 2 concentric arcs + dot |

### Modifier sync (split custom GATT)

Central writes `(full_mods >> 4) & 0x0F` (R-mod nibble) to peripheral on keycode events.
Peripheral bit mapping: `bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI`.

### Layer names (central widget)

```c
static const char *layer_names[] = {
    "Qwerty (Win)", "Qwerty (Mac)",
    "Colemak (Win)", "Colemak (Mac)",
    "Num", "Nav", "Sym", "Admin", "Func",
};
```

Layers 1 and 3 trigger Mac modifier order (⇧⌘⌃⌥ / ⌥⌃⌘⇧).

### Homerow mods

`hml` (left) / `hmr` (right): balanced flavor, 280 ms tapping-term, 175 ms quick-tap, 150 ms require-prior-idle, hold-trigger-on-release.

### Caps Word

Combo: both shift thumbs (positions 38+39), timeout 50 ms → `&caps_word`.

## Build & Flash

```bash
# CI builds on push via GitHub Actions (build.yaml)
# Local: requires west workspace with ZMK — not checked in here
# Flash: drag UF2 onto bootloader drive (double-tap reset)
```
