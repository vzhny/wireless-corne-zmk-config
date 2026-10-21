# Wireless Corne ZMK Config — Claude Notes

## Project

Boardsource Wireless SMT Corne split keyboard running ZMK firmware.
- Left half = central (BLE HID host, USB, ZMK Studio)
- Right half = peripheral (BLE split, no USB)
- Displays: nice!view (68×160 Sharp Memory LCD) on both halves
- Board: `blecorne_left` / `blecorne_right` (nRF52840, Adafruit UF2 bootloader)
- CI: GitHub Actions → `zmkfirmware/zmk` build-user-config workflow (reads `build.yaml` only)
- ZMK pinned to `f84ef436` (not tracking `main`) — see `config/west.yml` and `.github/workflows/build.yml`. Bump deliberately, then re-verify build.
- `README.md` has vendor-level hardware reference (product page, expansion-header
  pinout, battery connector/dimensions) not duplicated here — check it before wiring
  new hardware, rather than re-deriving pinout from the `.dtsi`.

## Display Preview

**Always read and update BOTH `docs/display_preview.html` (design reference) and
`docs/nice_view_pixel_mockup.html` (pixel-exact hardware reproduction) when working on
display widgets — any change to widget C code, layout, offsets, fonts, or text must be
mirrored in both mockups, not just one.**

- Both are self-contained HTML artifacts, JetBrains Mono embedded as base64 `@font-face`
  (`font-family: 'JBMono', 'JetBrains Mono', 'Courier New', monospace` on `body` and, in
  the pixel mockup, on the canvas `fillText` calls too) — all text in both mockups is
  monospace, matching the real device's aesthetic
- `display_preview.html` shows all left/right display states: BT status, battery,
  layer name, WiFi glyph
- `nice_view_pixel_mockup.html` is a byte-exact 1:1 recreation of what the current C code
  draws (canvas coordinates, `BASE` offsets, glyph bitmaps), overlaid on a real module
  photo — must be kept in lockstep with `config/widgets/*.c`, not just aspirational
- Publish `display_preview.html` to Artifact at `https://claude.ai/code/artifact/8b062b93-4ffb-4774-9e6f-c256a5bd9e2a`
- Font files for rebuilding: JBMono 400/700 TTF — look in session scratchpad or re-download from JetBrains

### Mockup rendering rules
- `nice_view_pixel_mockup.html` renders text/icons with the real embedded fonts
  (`PixelOperatorMono.ttf`, subsetted `JetBrainsMonoNLNerdFont-Regular.ttf`) via canvas
  `fillText`, then **hard-thresholds the alpha to binary** (`blendGlyph()`, alpha>=0.5)
  before blending. This is required, not optional: the real fonts are bpp=1 (no
  antialiasing at all), but the browser's own text rasterizer antialiases regardless of
  the source font's bpp - without the threshold, the mockup shows soft/fuzzy gray edges
  the real 1-bit hardware can never produce. Verified the underlying pixel buffer really
  is binary (only 0/255) via a headless-browser check, not just eyeballing a screenshot -
  screenshots of the CSS-scaled canvas can still look slightly soft at non-integer zoom
  factors (the photo-overlay view scales via a `0.34` transform) even when the source
  data is genuinely crisp; that's a display/zoom artifact, not a data problem.
- `nice_view_pixel_mockup.html`'s `drawText()` `fontPx` equals the `lv_font_conv --size`
  (browser px and `--size` give the same 0.5em advance: 20 → 10px/char, 24 → 12px/char -
  measured headless). Vertical placement differs (browser `'top'` baseline vs LVGL's
  `line_height`/`base_line` box), so `LAYER_TEXT_Y` (19) is tuned to land the layer
  name's ink on rows 27-40, the same rows firmware's `y=24` gives. Re-measure the ink
  rows headless if the font or y ever changes.
- `rasterizeIconInk()` centers icons by *measured* ink bounds (`TextMetrics.
  actualBoundingBox*`), not `fillText` alignment - advance width ≠ ink width for these
  glyphs (the same mismatch that mis-centers them under LVGL's `LV_TEXT_ALIGN_CENTER`).
- All glyphs rendered as pixel-art SVG (in `display_preview.html`) or pixel-art bitmap
  arrays (in `nice_view_pixel_mockup.html`) — NOT smooth curves — matching the actual L8 bitmap
- `display_preview.html`: plain string concatenation in JS — no template literals (backtick
  regex injection risk). `nice_view_pixel_mockup.html` uses template literals already;
  don't backport that to `display_preview.html`
- WiFi glyph: `wifiSvg(connected)` — 2× scale pixel-art, 28×28 viewBox in 14×14 CSS SVG
- BT glyph: `btSvg(vis)` — inline SVG with opacity for connected/disconnected

## Display Architecture

### Canvas layout (nice!view 68×160, rotated 270° CW before display)

| Canvas | Driver x offset | Physical region | Content |
|--------|----------------|-----------------|---------|
| `canvas_bot` | +92 (TOP_RIGHT) | Top strip — full 68px available, content only drawn in the first 24 | Status (BT, battery, profile / WiFi glyph) |
| `canvas_mid` | +24 | Middle 68 px | Layer name, size 24, `y=24` (central) / rules only (peripheral); 1px rules at `y=0`/`y=67` on both |
| `canvas_top` | −44 | Bottom strip — hardware-truncated to 24px, rest of the 68px canvas is silently dropped | Blank (both halves) |

Two independent constraints here, easy to conflate:
1. **Which offset lands where physically** is fixed by the rotation math: `-44` always
   lands in the slot with only 24px of framebuffer left before the 160px buffer ends
   (rows 0-23 of that canvas are visible, the rest is silently dropped); `+92`
   (TOP_RIGHT) lands in the slot with the full 68px available. This never changes.
2. **Which physical strip (top vs bottom) each offset corresponds to** is a separate,
   independently confirmed fact from a real-hardware photo: `-44` → physical **bottom**,
   `+92` → physical **top**. Confusing these two is what caused two rounds of bugs this
   session — first assuming `-44`=top (wrong, never verified), then assuming any canvas
   could go in either slot regardless of content (wrong when the layer canvas still had
   circles needing the full 68px; fine now that it's text-only).
- Status sits in the `+92`/top slot; the layer name sits in the `+24`/middle slot
  (moved there from the `-44`/bottom slot once the modifier cells were removed, so it
  can use the full 68px and a bigger font). The `-44` slot is blank on both halves -
  anything drawn below row 23 there would be silently clipped.

- Canvas is 68×68 px, L8 format; byte values follow `LVGL_FOREGROUND`/`LVGL_BACKGROUND`
  (util.h) — plain `lv_color_black()`/`lv_color_white()`, background=`0xFF`, ink=`0x00`
- `canvas_bot` isn't hardware-truncated (see above) so it isn't limited to 24 rows in
  practice - it currently draws through about row 36 (icon row at `y=0`, text row at
  `y=20`) now that the status icons/text have grown, well within its full budget
- After drawing, call `rotate_canvas()` (270° CW via `lv_draw_sw_rotate`)

There used to be a `NICE_VIEW_WIDGET_INVERTED` Kconfig symbol here (black background,
white ink) - removed. It never actually changed what showed up on real hardware (still
light background/dark ink either way, whatever the real nice!view driver does under the
hood), so `LVGL_BACKGROUND`/`LVGL_FOREGROUND` are now just plain white/black - no
Kconfig, no `IS_ENABLED()` branch. Don't re-add a color-inversion toggle without first
confirming on real hardware it does something.

### Key source files

```
config/
  blecorne.conf               — Kconfig values (display, BLE)
  blecorne.keymap             — Keymap (layers 0-7, combos, HRMs)
  CMakeLists.txt              — Build sources (display widgets)
  custom_status_screen.c      — ZMK display entry point
  widgets/
    util.h / util.c           — rotate_canvas, canvas_draw_* primitives
    fonts/
      pixel_operator_mono_large.h/.c — BT profile/battery %, size 20
      pixel_operator_mono_xl.h/.c    — Central-only: layer name, size 24
      status_icon_font.h/.c          — Status-row icons (bt/wifi/battery*5/bolt), size 18
      status_icon_font_wifi.h/.c     — Peripheral-only: wifi icon alone, size 16 (smaller)
    blecorne_central.c        — Left half widget (status + layer name)
    blecorne_peripheral.c     — Right half widget (status only)
boards/boardsource/blecorne/
  blecorne.dtsi               — Shared DTS: blue_led P1.09, ext_power P0.31, vbatt ADC
  blecorne_left.dts / _right.dts — Per-half kscan matrices
```

### Fonts (`config/widgets/fonts/`)

All fonts are converted with `lv_font_conv` (`npx lv_font_conv`) at `--bpp 1` (pure
black/white). This Sharp Memory LCD is strictly monochrome: a bpp=4 font's antialiased
edge pixels get thresholded and look mushy; bpp=1 stays crisp. Both source `.ttf` files
live at the repo root (`PixelOperatorMono.ttf`, `JetBrainsMonoNLNerdFont-Regular.ttf`);
each generated `.c` has its exact `Opts:` command line in its header comment.

| Font | Source | Size | Used for |
|------|--------|------|----------|
| `pixel_operator_mono_large` | PixelOperatorMono (CC0), ASCII | 20 (10px/char) | BT profile / battery `%` |
| `pixel_operator_mono_xl` | PixelOperatorMono, ASCII | 24 (12px/char) | Layer name (central) |
| `status_icon_font` | JetBrains Mono Nerd Font, listed PUA codepoints | 18 | BT, battery ×5, bolt |
| `status_icon_font_wifi` | same | 16 | Peripheral wifi/connection icon only |

- Size 24 is an exact 2× of Pixel Operator's pixel grid (every stroke a clean 2px). The
  longest layer name ("Admin", 5 chars) is 60px in the 68px canvas; size 26+ clips.
- Status icons: `ICON_BT` U+F294 `fa-bluetooth_b`, `ICON_WIFI` U+F1EB `fa-wifi`,
  `ICON_BATTERY_FULL/3_4/HALF/QUARTER/EMPTY` U+F240-F244, `ICON_BOLT` U+F0E7 `fa-flash`.
  `ICON_BOLT` once pointed at `md-bolt` (U+F0DB3), which is a screw, not a lightning
  bolt. **Verify any new codepoint by actually rendering it** (`fillText` on a
  `@font-face`'d span), never by its Nerd Fonts name alone.
- The wifi icon has its own smaller font because it is the widest glyph (`box_w=21` at
  size 18) and crowded its corner.
- `battery_icon()` (each widget .c) picks one of five discrete levels. At <=5% it shows
  `ICON_BATTERY_EMPTY`, drawn only while `flash_on` (blinks). While charging (central
  only - peripheral can't read USB power) `ICON_BOLT` replaces the battery icon.

There used to be modifier cells in `canvas_mid` (Shift/Ctrl/GUI/Alt boxes, `icon_font`,
`pixel_operator_mono` size 16, display-only "shadow-tracking" of the homerow positions,
and a custom split GATT service syncing the right-hand mods to the peripheral) -
**removed** (2026-10-20). Drawing them was cheap, but the plumbing sent a BLE write to
the peripheral on every keystroke plus a peripheral redraw, for an indicator that was
only an approximation of real HID state. Don't re-add without a reason that outweighs
that radio/battery cost.

**Flash state** (`flash_on`, both widgets): one 500ms timer, always running, toggles
`flash_on` and redraws the status strip. Drives the BT/connection icon's blink while
searching and the battery-empty blink at <=5%.

The BT/connection icon is **always drawn, never absent** - `active = connected ||
flash_on`, so while searching it blinks rather than vanishing permanently.

### Layer names (central widget)

```c
static const char *layer_names[] = {
    "Base", "Base", "Base",
    "Num", "Nav", "Sym", "Func", "Admin",
};
```

Layer order (`blecorne.keymap`): `0` qwerty_win, `1` qwerty_win_profile (fully
transparent - exists only as an explicit Admin `&tog 1` target so Qwerty Win is
selectable symmetric with the other profile, since layer 0 itself can't safely be
toggled), `2` qwerty_mac, `3` num, `4` nav, `5` sym, `6` func, `7` admin. Admin's
homerow-left toggles: `tog 1`=Qwerty Win, `tog 2`=Qwerty Mac - both runtime-only (off
on power cycle).

This repo used to also have Colemak-DH Win/Mac base layers (`colemak_win`/`colemak_mac`,
indices 3-4, toggled via `tog 3`/`tog 4`) - **removed** (2026-08-05) to keep Qwerty
(Win + Mac) only, mirroring the same change made in the KLOR sibling config in the
opposite direction (Qwerty removed there, Colemak-DH kept). Intentional, not a
regression - re-adding Colemak here means re-inserting two base layers before NUM,
which renumbers every layer index below and every reference to it (conditional_layers,
combos' `layers = <...>`, `LAYER_QWERTY_MAC`, `layer_names[]`) - redo the full
old→new layer table exercise rather than patching numbers by feel.

Layers 0-2 (Qwerty Win/Win-profile/Mac) all show **"Base"** - drawn at
`pixel_operator_mono_xl` in the middle strip. The display doesn't distinguish Win vs
Mac.

**Func is 6, Admin is 7 - Admin must be the higher index.** `zmk_keymap_highest_layer_active()`
(used by `layer_event_cb` here, for the layer-name text only) returns whichever active
layer has the highest numeric index, not the most-recently-activated one. Admin is a
conditional layer (`blecorne.keymap`'s `conditional_layers`, `if-layers = <3 6>` NUM+FUNC
or `<3 4>` NUM+NAV, `then-layer = <7>`) that stacks on top of whatever momentary layers
triggered it - if Admin's index were *lower* than Func's, holding NUM+FUNC would
correctly activate Admin's keymap bindings but the display would still say "Func".
Confirmed on real hardware. If a new conditional layer is ever added, give it the
highest index of anything it can combine with, not just the next free number.

**Combos' `layers = <...>` Win/Mac split must check the profile layer, not the
highest-active layer** - NUM/NAV/SYM/FUNC/ADMIN (3-7) all stack above the profile
layers (0-2), so anything keyed off `zmk_keymap_highest_layer_active()` silently reads
"not Mac" while one of them is held. `widget_state.active_layer`
(`highest_layer_active()`) is only used for the layer-*name* text, where "show
whichever is highest" is correct.

No layer-number circles (removed - too small to read).

### Peripheral's middle and bottom strips

`render_static_canvases()` (`blecorne_peripheral.c`) draws the two section rules in
`canvas_mid` and clears `canvas_top`, once at init. The peripheral has no keymap/layer
state of its own (ZMK doesn't run the keymap there), so it has nothing to show in these
strips without a custom split sync - which was removed along with the mod cells.

### Status strip (`canvas_bot`, both halves)

Two rows, not one: **row 1 (y=0)** is icons only - BT/wifi icon left-aligned, battery/
bolt icon right-aligned - `draw_status_icon(..., LV_TEXT_ALIGN_LEFT/RIGHT)`. **Row 2
(y=20)** is text, `pixel_operator_mono_large` (same size as the layer name), directly
under its own icon - BT profile left-aligned (field width 22, `x=0`), battery `%`
right-aligned (field width 40). Peripheral's row 2 has no left-side text (no
BT-profile equivalent), just the right-aligned `%`.

- **Field widths are sized off exact character counts, not guessed.**
  `pixel_operator_mono_large` is a fixed 10px/char, so `"100%"` (4 chars) is always
  40px wide - already more than half the 68px canvas. That's why `"BT %d"` (5 chars,
  50px) had to shrink to `"B%d"` (max `"B4"`, 2 chars, 20px): at this font size the two
  fields plus even a minimal gap don't fit in 68px otherwise. This was a deliberate
  trade-off (asked the user rather than silently picking): abbreviate the BT label,
  vs. keep the smaller font, vs. accept a near-zero gap. Abbreviating won. Don't
  "restore" `"BT %d"` without redoing this width math first - it will silently
  overflow/clip against the battery field again.
- **Icon x/y offsets are real-hardware-calibrated, not derived from the field-width
  math above - don't "clean them up" back to round numbers.** First-flash testing on
  actual nice!view modules found: central's battery/bolt icon sitting slightly past
  the true right edge (nudged from `x=44` to `x=40`, battery/bolt icon only - the row 2
  `%` text position was already fine, left as-is). A second round of hardware testing
  found `x=40` still clipped off the right edge - nudged again to `x=36` (edgeInset 8),
  matching the correction magnitude the peripheral's battery icon already needed (below)
  rather than being a smaller, central-only fudge - if this ever needs another nudge,
  check whether the peripheral needs the same change first before guessing a new value;
  peripheral's battery icon
  (`x=44`→`x=36`) and its `%` text (`x=28`→`x=20`) moved together by the same 8px,
  worse than central's needed since that's the half with a real battery installed and
  it was genuinely clipping off the display edge; peripheral's wifi icon (`x=0,y=0` →
  `x=3,y=3`) was touching the physical top-left corner. None of this shows up in the
  mockup's own from-scratch ink-measurement math (browser font metrics don't have the
  same offset LVGL's do) - the mockup's `drawIcon()` takes explicit `edgeInset`/`topPad`
  params specifically to carry these hardware-only corrections without disturbing its
  own otherwise-correct centering.
- Central only: while searching for a BLE connection, the BT glyph blinks dim/bright
  every 500ms (`flash_on`) instead of showing "Connecting..." text — no text is drawn
  on row 2 until actually connected (`bt_connected`). It's never fully absent, even
  when disconnected - dim (`LV_OPA_40`) rather than not drawn at all, matching the
  peripheral's wifi icon (dim when disconnected, full white when connected - see
  `draw_wifi_icon`).
- See Fonts above for the battery icon (`status_icon_font`, discrete levels + blink,
  now size 18) and the two section-separator rules drawn from `canvas_mid`.

### Homerow mods

`hml` (left) / `hmr` (right): balanced flavor, 200 ms tapping-term, 175 ms quick-tap,
150 ms require-prior-idle, `hold-trigger-key-positions` (opposite hand + thumbs).

**No `hold-trigger-on-release`** (removed from both). That flag deferred the tap/hold
decision until the hold-tap key itself was released, instead of resolving via the
tapping-term timeout or an opposite-hand `hold-trigger-key-positions` press - confirmed
on real hardware this made the display only light up a held modifier *after* releasing
it, and made holding multiple homerow mods resolve one at a time instead of
simultaneously.

That fix was necessary but not sufficient - ZMK's `"balanced"` flavor itself only
resolves a hold-tap key in one of two ways: the tapping-term timeout elapses while it's
held alone, or another ("interrupting") key is pressed *and released* while it's still
held. Critically, resolution happens on the interrupting key's **release**, not its
press, and ZMK queues both keys' events internally until that decision is made - so
neither reaches the display (or HID) until then. This is inherent to `"balanced"`, not
something `hold-trigger-on-release` caused, and it's still true after that fix.

`thm` (thumb shift/key, tap = key, hold = modifier, tap-then-hold = repeat key): also
`"balanced"`, 200 ms tapping-term, 175 ms quick-tap. Briefly tried `"hold-preferred"`
here to get faster resolution, but confirmed on hardware it didn't meaningfully help and
made this key behave differently from `hml`/`hmr` for no real benefit - reverted.
**Don't change `hml`/`hmr`/`thm`'s flavor or other tap/hold timing without the user
asking** - these are tuned to how the user actually types (tapping-term last set to
200 ms across all three, per explicit request) and are deliberately out of scope for
the "modifiers don't show in real time" problem; solve that on the display side (see
below) instead of by changing what these keys actually do.

### Caps Word

Combo: both shift thumbs (positions 38+39), timeout 50 ms → `&caps_word`.

## Build & Flash

```bash
# CI builds on push via GitHub Actions (build.yaml)
# Download: grab firmware.uf2 (left/right) from the Actions run's artifacts tab
# Local: requires west workspace with ZMK — not checked in here
# Flash: drag UF2 onto bootloader drive (double-tap reset)
```

**Why `config/CMakeLists.txt` (custom_status_screen.c, all widget/font sources)
actually gets compiled, when nothing in `build.yaml`/`west.yml` says so directly:**
`ZMK_CONFIG` only wires up `.conf`/`.overlay`/keymap files, never a `CMakeLists.txt` —
what pulls this repo's `config/CMakeLists.txt` into the build is `zephyr/module.yml`
(`build.cmake: config`), which registers this repo as a Zephyr module via
`ZMK_EXTRA_MODULES`. Missing that key means the custom display/split code silently
never compiles — no error, just `zmk_display_status_screen()` falling back to ZMK's
weak-stub default (`<err> zmk: No status screen provided` at runtime). This was the
root cause of a real multi-day first-flash debugging session; don't remove or
"simplify" `zephyr/module.yml` without understanding this.
