# Wireless Corne ZMK Config — Claude Notes

## Project

Boardsource Wireless SMT Corne split keyboard running ZMK firmware.
- Left half = central (BLE HID host, USB, ZMK Studio)
- Right half = peripheral (BLE split, no USB)
- Displays: nice!view (68×160 Sharp Memory LCD) on both halves
- Board: `blecorne_left` / `blecorne_right` (nRF52840, Adafruit UF2 bootloader)
- CI: GitHub Actions → `zmkfirmware/zmk` build-user-config workflow (reads `build.yaml` only)
- ZMK pinned to `f84ef436` (not tracking `main`) — see `config/west.yml` and `.github/workflows/build.yml`. Bump deliberately, then re-verify build.

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
  modifiers, layer circles, WiFi glyph
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
- `nice_view_pixel_mockup.html`'s modifier-cell border/fill (`drawModBox`) uses canvas
  2D's `roundRect()` rasterized through the same threshold-to-binary pipeline as text/
  icons (`rasterizeRoundRect()` + `blendGlyph()`), not manual pixel math - keeps the
  border crisp and consistent with everything else. `blendGlyph()` takes an optional
  `color` param (defaults to `FG`) specifically so pressed modifier cells can blend
  content in `BG` instead (the invert-on-press look) - don't hardcode `FG` in new
  callers if the content might need to invert.
- `nice_view_pixel_mockup.html`'s `drawModIcon()` centers icons manually (`ICON_W` +
  computed `contentX`), not via `fillText` `align: 'center'` - canvas `measureText()`
  returns the font's advance width, not tight ink bounds, the identical mismatch that
  makes LVGL's `LV_TEXT_ALIGN_CENTER` mis-center these same glyphs in firmware (see
  Fonts above). Keep `ICON_W` in sync with `icon_font.h`'s `ICON_*_W` macros if the
  icon font is ever regenerated.
- All glyphs rendered as pixel-art SVG (in `display_preview.html`) or pixel-art bitmap
  arrays (in `nice_view_pixel_mockup.html`) — NOT smooth curves — matching the actual L8 bitmap
- `display_preview.html`: plain string concatenation in JS — no template literals (backtick
  regex injection risk). `nice_view_pixel_mockup.html` uses template literals already;
  don't backport that to `display_preview.html`
- WiFi glyph: `wifiSvg(connected)` — 2× scale pixel-art, 28×28 viewBox in 14×14 CSS SVG
- Alt glyph: `altSVG()` — 2× scale pixel-art, `fill="currentColor"` for active/inactive states
- BT glyph: `btSvg(vis)` — inline SVG with opacity for connected/disconnected

## Display Architecture

### Canvas layout (nice!view 68×160, rotated 270° CW before display)

| Canvas | Driver x offset | Physical region | Content |
|--------|----------------|-----------------|---------|
| `canvas_bot` | +92 (TOP_RIGHT) | Top strip — full 68px available, content only drawn in the first 24 | Status (BT, battery, profile / WiFi glyph) |
| `canvas_mid` | +24 | Middle 68 px | Modifier glyphs |
| `canvas_top` | −44 | Bottom strip — hardware-truncated to 24px, rest of the 68px canvas is silently dropped | Layer name (central, text only, no circles) / blank (peripheral) |

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
- Status now sits in the `+92`/top slot; layer name sits in the `-44`/bottom slot. Layer
  name text was moved from `y=27` (centered in a full 68px canvas) to `y=5` (centered in
  just the 24 visible rows) to match — text placed below row 23 in that slot would be
  silently clipped, same as it would be for status content.

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
  blecorne.keymap             — Keymap (layers 0-8, combos, HRMs)
  CMakeLists.txt              — Build sources (display widgets)
  custom_status_screen.c      — ZMK display entry point
  widgets/
    util.h / util.c           — rotate_canvas, canvas_draw_* primitives
    fonts/
      pixel_operator_mono.h/.c       — "Ctl"/"Win"/"Alt" mod-cell text only, size 16
      pixel_operator_mono_large.h/.c — Layer name, layout name, BT profile/battery %, size 20
      icon_font.h/.c                 — Modifier-row icons (shift/ctrl/cmd/opt), size 24
      status_icon_font.h/.c          — Status-row icons (bt/wifi/battery*5/bolt), size 18
      status_icon_font_wifi.h/.c     — Peripheral-only: wifi icon alone, size 16 (smaller)
    blecorne_central.c        — Left half widget (status + mods + layer name)
    blecorne_peripheral.c     — Right half widget (status + mods + layout name)
  split/
    modifier_sync_central.c   — BLE GATT client: writes R-mod nibble + is_mac + is_colemak to peripheral
    modifier_sync_peripheral.c — BLE GATT server: receives that byte, calls update_mods
boards/boardsource/blecorne/
  blecorne.dtsi               — Shared DTS: blue_led P1.09, ext_power P0.31, vbatt ADC
  blecorne_left.dts / _right.dts — Per-half kscan matrices
```

### Fonts (`config/widgets/fonts/`)

All hand-drawn 0/1 bitmap glyphs (`glyphs.c`, deleted) and LVGL's built-in Montserrat
(bpp=4, antialiased) were replaced with fonts converted via `lv_font_conv`
(`npx lv_font_conv`), all at `--bpp 1` (pure black/white, no antialiasing). This
project's Sharp Memory LCD is strictly monochrome - a bpp=4 font's antialiased gray
edge pixels get thresholded on that display and look soft/mushy; a bpp=1 font has no
antialiasing to lose, so it stays crisp regardless of size. Both source `.ttf` files
live at the repo root (`PixelOperatorMono.ttf`, `JetBrainsMonoNLNerdFont-Regular.ttf`)
for re-running the conversion if a codepoint/size needs to change.

**Two `pixel_operator_mono*` fonts, one source TTF, two sizes** - each screen element
gets whichever size actually fits it, rather than compromising on one:

| Font | Size | `adv_w`/`line_height` | Used for |
|------|------|------------------------|----------|
| `pixel_operator_mono` | 16 | 8px/char, 13px | "Ctl"/"Win"/"Alt" mod-cell text only |
| `pixel_operator_mono_large` | 20 | 10px/char, 16px | Layer name, peripheral's layout name, BT profile/battery `%` - bigger for readability |

There used to be a third, `pixel_operator_mono_small` (size 10), for the peripheral's
layout row when it showed `"QWERTY"`/`"COLEMAK-DH"` - removed once that text shortened
to `"Qwerty"`/`"Colemak"` and moved to `pixel_operator_mono_large` to match the layer
name's size (see Status strip below). Both remaining fonts come from
`PixelOperatorMono.ttf` (CC0, a font actually designed pixel-by-pixel for bitmap
displays), `-r 0x20-0x7F` (ASCII).

**`icon_font`** and **`status_icon_font`** — both from `JetBrainsMonoNLNerdFont-Regular.ttf`
(MIT, via [Nerd Fonts](https://github.com/ryanoasis/nerd-fonts), which patches Material
Design Icons + Font Awesome glyphs into the Private Use Area of any base font), specific
PUA codepoints only (not a full range - keeps each font tiny). **Two separate fonts, not
one**: a single shared size (originally 24, reused for everything) clipped/overflowed the
status strip - `icon_font` is `--size 24` for the modifier row's 28px cells,
`status_icon_font` is `--size 18` (bumped up from an initial `--size 12` that read as
"way too small" once compared against the reference implementation) to read clearly in
the status row, which has the full 68px canvas to grow into (see Status strip below).
Same source TTF, just converted twice with different `--size`/`-r` codepoint sets.
Font Awesome Free has **no** Mac keyboard glyphs (⌘⌥⇧⌃) at all; Nerd Fonts' Material
Design set does (`md-apple_keyboard_*`), and those look better-designed than the raw
Unicode Miscellaneous Technical symbols anyway.

`icon_font.h` (modifier row, size 24):

| Symbol | Macro | Codepoint | Source glyph | ink width (`ICON_*_W`) |
|--------|-------|-----------|--------------|------------------------|
| ⇧ Shift | `ICON_SHIFT` | U+F005D | `md-arrow_up` | 16 |
| ⌃ Ctrl | `ICON_CTRL` | U+F0634 | `md-apple_keyboard_control` (shared Win+Mac) | 16 |
| ⌘ Cmd | `ICON_CMD` | U+F0633 | `md-apple_keyboard_command` | 20 |
| ⌥ Opt | `ICON_OPT` | U+F0635 | `md-apple_keyboard_option` | 18 |

`ICON_SHIFT` isn't `md-apple_keyboard_shift` (U+F0636) anymore - that glyph is a
solid/filled arrow-with-roof shape, visibly heavier-stroked than the "Ctl"/"Win"/"Alt"
text and the other 3 (thin-lined) icons, confirmed on real hardware. This font has no
outlined/thin variant of that specific shape. Went through two intermediate choices
before landing here: `md-chevron_up` (U+F0143) fixed the stroke weight but then
looked nearly identical to `ICON_CTRL` (also a plain "^") at this size; `fa-angles_up`
(U+F102, a double chevron "^^") fixed that but was a poorer match for the actual Mac ⇧
glyph shape. `md-arrow_up` (arrowhead + stem) is thin-stroked like the rest, distinct
from Ctrl's bare caret, and closer to what ⇧ actually looks like. If a source glyph is
ever swapped again, actually render and compare it (`fillText` on a `@font-face`'d
span, not just the Nerd Fonts name) rather than assuming - "looks about right" from
the name alone is exactly how the original `md-bolt` mixup (see Fonts below, status
row) and this one both happened.

`fa-windows` (U+F17A, formerly `ICON_WIN`) was dropped from the conversion entirely -
nothing ever referenced it, since Windows shows "Win" as text (see below), never an icon.

Each icon's `ICON_*_W` macro is its real ink width (`box_w` in the generated
`icon_font.c`); `ICON_*_OFS_X` is its left bearing (`ofs_x`, currently 0 for all 4,
kept as named constants rather than assumed so a future glyph swap with nonzero
bearing doesn't silently mis-center). Needed because `LV_TEXT_ALIGN_CENTER` centers by
the font's shared `adv_w` (~14px, the same for every glyph in this font), not each
glyph's actual `box_w` (16-20px, wider than `adv_w`), so centering-by-advance-width
visibly shifts these icons off-center. `draw_mod_icon()` in both widget `.c` files
centers manually instead: `LV_TEXT_ALIGN_LEFT` at `x + (MOD_BOX_W - icon_w) / 2 -
icon_ofs_x` horizontally. Vertically it's just a flat `y + 3` for all four glyphs, not
a per-glyph adjustment - working through LVGL's glyph-placement formula by hand
(`baseline_y = y + line_height - base_line`, `glyph_top = baseline_y - ofs_y - box_h`)
shows this already centers all four within about half a pixel of the box center,
despite their differing `box_h`/`ofs_y`, so no per-icon vertical constant was needed.
The pixel mockup solves the equivalent centering problem more directly, since it has
no such font-metrics table to hand-derive from: `rasterizeIconInk()` uses the
*measured* ink bounds from canvas `TextMetrics.actualBoundingBox{Left,Right,
Ascent,Descent}` to center both axes exactly, rather than reproducing LVGL's box math in
JS. Prefer that measured-ink approach for any *new* mockup icon-drawing code - it's more
robust than a hand-maintained constants table and was what actually fixed a previous
round's "icons still slightly off-center, sitting low" bug (the old mockup code used
`fillText`'s own `'center'` alignment plus a guessed vertical offset, neither of which
was ink-accurate).

`status_icon_font.h` (status row, size 18):

| Symbol | Macro | Codepoint | Source glyph |
|--------|-------|-----------|--------------|
| ᛒ→ BT | `ICON_BT` | U+F294 | `fa-bluetooth_b` |
| 📶 WiFi | `ICON_WIFI` | U+F1EB | `fa-wifi` |
| 🔋 Battery 76-100% | `ICON_BATTERY_FULL` | U+F240 | `fa-battery_full` |
| 🔋 Battery 51-75% | `ICON_BATTERY_3_4` | U+F241 | `fa-battery_three_quarters` |
| 🔋 Battery 26-50% | `ICON_BATTERY_HALF` | U+F242 | `fa-battery_half` |
| 🔋 Battery 6-25% | `ICON_BATTERY_QUARTER` | U+F243 | `fa-battery_quarter` |
| 🔋 Battery ≤5% | `ICON_BATTERY_EMPTY` | U+F244 | `fa-battery_empty` (blinks, see below) |
| ⚡ Charging | `ICON_BOLT` | U+F0E7 | `fa-flash` (this Nerd Font's name for Font Awesome's bolt glyph) |

`ICON_BOLT` used to point at `md-bolt` (U+F0DB3) - a screw/fastener icon, not a
lightning bolt. Verify any new codepoint choice by actually rendering the glyph
(`compare_bolt.html`-style check) rather than trusting a Nerd Fonts name alone -
`md-bolt` sounded right and wasn't.

Peripheral's wifi/connection icon is a **separate, smaller font**,
`status_icon_font_wifi.h/.c` (size 16, same U+F1EB codepoint, `ICON_WIFI_SMALL`) - it's
the widest glyph in `status_icon_font` (`box_w=21` at size 18) and read as crowding its
corner at the same size as BT/battery/bolt on the other half. `draw_wifi_icon()` in
`blecorne_peripheral.c` uses this font instead of `status_icon_font`; nothing else
needed a second size since only one glyph out of the set was too wide.

**Windows uses text for Ctrl/GUI/Alt, icons for Shift only; Mac uses icons for all
four.** Real Windows keyboards print "Ctrl"/"Win"/"Alt" as **text** on the keycap, not
a symbol - Mac keyboards do print ⌘/⌥/⌃ symbols (and, less commonly, ⇧), so Mac stays
all-icon. `render_mod_canvas` branches on `is_mac`: Mac calls `draw_mod_icon()` for
Shift/Ctrl/Cmd/Opt; Windows calls `draw_mod_icon()` for Shift only and `draw_mod_text()`
(`"Ctl"`/`"Win"`/`"Alt"`, `pixel_operator_mono`) for the rest - "Ctrl" (4 chars) wraps
onto a second line at this box/font size, so it's "Ctl" (3 chars, matching "Win"/"Alt")
instead. This is a deliberate asymmetry, not a gap to fill later - don't "fix" Windows
to use icons for consistency.

Battery is `status_icon_font` glyphs now, not hand-drawn rects - `draw_battery` was
removed from `util.c` entirely. `battery_icon()` (in each widget .c) picks one of five
discrete Font Awesome levels from the percentage (not a continuous fill like the old
hand-drawn shell+bar). At <=5% the icon is `ICON_BATTERY_EMPTY` and only drawn while
`flash_on` (blinks - see below); while charging, the battery icon is replaced entirely
by `ICON_BOLT` (nothing battery-related drawn at all, central only - peripheral has no
charging state to read).

**Flash state** (`flash_on`, both widgets): a single 500ms-heartbeat timer, always
running (not started/stopped based on BT connection like the old `bt_flash_on` was),
driving two independent blinks: the BT glyph while searching (central only) and the
battery-empty glyph at <=5% (both halves). One shared timer/bool since both just need
"on/off every 500ms", not separate schedules - had to switch from "stop the timer once
BT connects" to "always running" since battery-empty needs the blink regardless of BT
state.

The BT icon itself is **always drawn, never absent** - `draw_status_icon(canvas_bot, 0,
0, 24, ICON_BT, bt_connected || flash_on, LV_TEXT_ALIGN_LEFT)` runs unconditionally now
(no `if` gate). Dim (`LV_OPA_40`) when disconnected, full white (`LV_OPA_COVER`) once
connected - matching the peripheral's wifi icon, which was already always-drawn with
this same dim/bright-by-`active` treatment (`draw_wifi_icon`, see Fonts above). While
searching (not yet connected), `active` alternates with `flash_on` every 500ms, so the
icon blinks dim/bright rather than sitting statically dim or vanishing - a prior
version only called `draw_status_icon` `if (bt_connected || flash_on)`, which made the
icon disappear entirely for half of each blink cycle instead of dimming.

**Modifier cells** (`draw_mod_box`/`draw_mod_icon`/`draw_mod_text` in each widget .c):
each modifier key is a `MOD_BOX_W`×`MOD_BOX_H` (30×26) box with a 1px rounded border
(`radius=6`), **always visible** regardless of press state. Pressed ("active") is shown
by filling the box with foreground color and inverting the icon/text color inside it to
background color (a "lit key" look) - replacing the old plain opacity fade
(`LV_OPA_COVER`/`LV_OPA_40`) entirely. Unpressed = transparent fill + foreground content
+ border only.

Modifier canvas (`canvas_mid`) layout: two rows of these cells - Shift+Ctrl on row 1
(y=6), GUI+Alt on row 2 (y=38), columns at x=4/x=36. Same arrangement on both halves
(peripheral doesn't mirror right-to-left - with only 2 columns instead of the old 4,
mirroring added complexity without a real benefit). This canvas also draws two 1px
horizontal rules (`y=0` and `y=67`, full width) as section separators between the
status/mods/layer strips - safe to draw both from this one canvas since it uses its
full 68px budget with no hardware truncation, landing exactly on the true physical
boundaries on both sides (see Canvas layout above). A similar rule at the status or
layer canvas's own edge would NOT line up the same way - those two are either only
partially used (status) or hardware-truncated (layer).

### Modifier sync (split custom GATT)

Central writes a single byte to peripheral (on keycode *and* layer events - layer
changes affect bits 4-5, not just the R-mod nibble):
- Bits 0-3: R-mod nibble, `(full_mods >> 4) & 0x0F`. Peripheral bit mapping:
  `bit0=RCtrl, bit1=RShift, bit2=RAlt, bit3=RGUI`.
- Bit 4: `is_mac` (`active_layer == 1 || active_layer == 3`) - Mac vs Win glyph order.
- Bit 5: `is_colemak` (`active_layer == 2 || active_layer == 3`) - drives the
  peripheral's layout-name row (`"Qwerty"`/`"Colemak"`).

Peripheral has no local keymap/layer state of its own (see the ZMK compile-guard note
elsewhere in this doc), so both flags have to be forwarded - this byte is peripheral's
only source of truth for anything layer-related. `blecorne_peripheral_update_mods()`
only re-renders the layout row when `is_colemak` actually *changes* (not on every
keycode), since that GATT write happens on every keystroke but the flag itself rarely
flips.

### Layer names (central widget)

```c
static const char *layer_names[] = {
    "Base", "Base",
    "Base", "Base",
    "Num", "Nav", "Sym", "Func", "Admin",
};
```

Layers 0-3 (Qwerty/Colemak × Win/Mac) all show **"Base"** now, not "Qwerty"/"Colemak" -
drawn at `pixel_operator_mono_large` (bigger, for readability), and at 10px/char even
"Colemak (Win)"-style full names never fit, let alone with a suffix. The two
distinctions this used to carry moved elsewhere instead of being dropped: Win vs Mac is
shown by the modifier row's icons-vs-text (see Fonts above), Qwerty vs Colemak by the
peripheral's new layout-name row (see Modifier sync below) - central has no equivalent
row of its own for this since its layer canvas is fully occupied by "Base" or the
non-base layer names (Num/Nav/etc).

**Func is 7, Admin is 8 - Admin must be the higher index.** `zmk_keymap_highest_layer_active()`
(used by both `layer_event_cb` here and `send_mod_state()` in `modifier_sync_central.c`)
returns whichever active layer has the highest numeric index, not the most-recently-
activated one. Admin is a conditional layer (`blecorne.keymap`'s `conditional_layers`,
`if-layers = <4 7>` NUM+FUNC or `<4 5>` NUM+NAV, `then-layer = <8>`) that stacks on top
of whatever momentary layers triggered it - if Admin's index were *lower* than Func's
(as it originally was: Admin=7, Func=8), holding NUM+FUNC would correctly activate
Admin's keymap bindings but the display would still say "Func", since 8 > 7. Confirmed
on real hardware before the swap. If a new conditional layer is ever added, give it the
highest index of anything it can combine with, not just the next free number.

Layers 1 and 3 trigger Mac modifier order (⇧⌘⌃⌥ / ⌥⌃⌘⇧). Layers 2 and 3 trigger
`is_colemak` for the peripheral's layout row.

### Layout name (peripheral widget)

`render_layout_canvas()` draws `"Qwerty"`/`"Colemak"` (not `"QWERTY"`/`"COLEMAK-DH"` -
shortened so it actually reads at this size) in `canvas_top`, same font, size, and `y`
as central's layer name (`pixel_operator_mono_large`, `y=5`) so the two halves read as
a matching pair. This used to be its own dedicated small font
(`pixel_operator_mono_small`, size 10) sized just to fit the longer strings - removed
once the strings themselves shortened, since reusing `pixel_operator_mono_large`
needed no separate font at all.

No layer-number circles anymore (removed — at 10px they were too small to read
reliably; `draw_circle` was deleted from `util.c`/`util.h` since nothing called it
after removal, and `CONFIG_LV_FONT_MONTSERRAT_8` dropped from `blecorne.conf` since the
circle numbers were its only user). The layer canvas is just the centered name — and
since it now sits in the 24-visible-row slot (see Canvas layout above), the text draws
at `y=5`, not vertically centered in the full 68px canvas.

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
  `%` text position was already fine, left as-is); peripheral's battery icon
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

`hml` (left) / `hmr` (right): balanced flavor, 280 ms tapping-term, 175 ms quick-tap,
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
something `hold-trigger-on-release` caused. `hml`/`hmr` keep `"balanced"` regardless
(chosen deliberately for typing accuracy, not to be changed) - `hold-trigger-key-positions`
gives same-hand/opposite-hand rolls an early-resolution path that sidesteps this for
the common case, which is why homerow mods feel fine in practice.

`thm` (thumb shift/key, tap = key, hold = modifier, tap-then-hold = repeat key):
**`"hold-preferred"`** (not balanced), 225 ms tapping-term, 175 ms quick-tap, no
`hold-trigger-key-positions`. Switched from balanced after confirming on hardware that
holding a thumb-shift key while typing didn't light up (or apply) the modifier until
releasing the interrupting key - `hold-preferred` resolves the instant an interrupting
key is *pressed*, which is what real-time chorded-shortcut use needs. Trade-off: rolling
this key with an adjacent one during fast normal typing is now more likely to register
as a hold than a tap - if that becomes annoying in practice, that's the knob to revisit
(tighten `tapping-term-ms`, or add `hold-trigger-key-positions` to exempt specific
same-hand keys from early-hold resolution).

### Caps Word

Combo: both shift thumbs (positions 38+39), timeout 50 ms → `&caps_word`.

## Build & Flash

```bash
# CI builds on push via GitHub Actions (build.yaml)
# Local: requires west workspace with ZMK — not checked in here
# Flash: drag UF2 onto bootloader drive (double-tap reset)
```
