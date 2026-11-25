# Sober Groove architecture

Version 0.1.0-alpha. For developers who want to build, change or port the firmware.

## 1. Overview

```
┌─────────────────────────── firmware/src/main.c (unity build) ───────────────────────────┐
│ HAL (firmware/hal)  libc.c  lcd.c  gfx.c  usb.c  storage.c  ota.c  midi_uart.c            │  platform
│ app/panel.c  app/plat_fm1.c  (plat.h: keys, buttons, encoders, LEDs, storage, clock)       │  (upstream)
├────────────────────────────────────────────────────────────────────────────────────────────┤
│ app/project.c  ← includes app/sg_state.c        save format, game state                    │
│ app/ui.c       ← includes sg_art.h, sg_text.h   screens, input, SOS, lessons, autosave      │  Sober Groove
│ app/main_fm1.c                                   main loop: audio ISR, ui_frame()           │
│ dsp/omni.c (separate unit, -O2)                  sound engine, om_render() in the audio ISR  │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

The firmware is one translation unit (`main.c` includes everything) plus the sound engine, compiled separately at `-O2`. The host simulator (`host/omni_host.c`) includes the same app sources against a simulated `plat.h`, so the whole UI, game state, drawing and sound run and are tested on a computer.

Everything below `firmware/src/app/{ui.c,sg_*,project.c}` and `firmware/src/dsp/omni.c` is unchanged from the upstream platform ([Trumpagotchi](https://github.com/adamcharleswilsonmusic-code/Trumpagotchi) → FoMni → X0X → Felucca). Only the identity string in `main.c` and the full-screen canvas size in `gfx.c` changed.

## 2. Threads of execution

| Context | Runs | Rules |
| --- | --- | --- |
| Audio interrupt | `om_render()`: 256 stereo frames at 44.1 kHz per call | Owns all DSP state. Reads commands from the ring. No drawing, no storage. |
| Main loop | `ui_frame()` every ≥16 ms, `ui_input_only()` in between | Input, game logic, drawing, saving. The only producer of sound commands. |

The UI posts sound commands into a 128-entry lock-free ring (`post()` in `omni.c`): one writer (main loop), one reader (audio ISR). Key releases use per-key generation counters that cannot be lost when the ring is full, so a key can never hang.

## 3. The UI state machine (`ui.c`)

One `ui` struct holds the screen, the selection, timers and the SOS and lesson state. `go(screen)` switches screens, resets the selection and asks for a redraw.

| # | Screen | Notes |
| --- | --- | --- |
| 0 | `S_SPLASH` | 3 s on every start |
| 1–11 | `S_WHAT` … `S_READY` | First start: intro, safety, choose buddy, prologue (4–9), help, ready |
| 12 | `S_MAIN` | Home |
| 13, 14 | `S_STATUS`, `S_STATUS2` | Status, next stage |
| 15 | `S_NEWDAY` | "A new day since we last talked?" |
| 16, 17 | `S_DECLARED`, `S_DONE_TODAY` | After the morning hold |
| 18–20 | `S_THOUGHT`, `S_ABOUT_PRO`, `S_ABOUT_GROUPS` | Thoughts and their MORE pages |
| 21 | `S_MOOD` | Talk: 1–5 |
| 22–26 | `S_SOS`, `S_HALT`, `S_TIP`, `S_SOS_EXIT`, `S_SOS_DONE` | The wave |
| 27 | `S_MENU` | GLO menu |
| 28–30 | `S_JAM_REC`, `S_JAM_LISTEN`, `S_JAM_RESULT` | A lesson |
| 31 | `S_REP` | Repertoire |
| 32, 33 | `S_HELP`, `S_GROUPS` | Help |
| 34–38 | `S_SLIP_*` | Slip flow |
| 39 | `S_SETTINGS` | |
| 40 | `S_LEVELUP` | Stage up, inverted |

The numbers are what `expect screen N` checks in the simulator.

### Input

`input()` runs on every loop pass. It edge-detects keys and buttons against the previous state and dispatches:

- white keys → `on_white()` (sound, lesson recording);
- black keys → `on_black()` (care, HALT answers, SOS);
- `SEL` → `on_sel()`, `GLO` → `on_glo()`;
- `HOME`: a tap on release (`on_home_tap()`), a hold of 1.5 s (`on_home_hold()`);
- black key 5 held 2 s during a wave → the early-exit question;
- the SELECT encoder moves the selection (`opt_count()` gives the number of options per screen).

It then advances the score player, the SOS wave, the lesson, the game state and autosave.

### Timing constants

| Constant | Value | Where |
| --- | --- | --- |
| `HOME_HOLD_MS` | 1500 ms | "not today" |
| `SOS_HOLD_MS` | 2000 ms | early exit from SOS |
| `SOS_MS` | 20 min | a wave |
| `HALT_AT_MS` | 5 min | HALT check |
| `HALFWAY_MS` | 10 min | halfway line (20 s) |
| `BREATH_MS` | 10 s | 5 s in with the bass, 5 s out |
| `ANIM_MS` | 450 ms | animation frame |
| `DAY_MS` | 20 h | next day while powered (`sg_state.c`) |
| `DECAY_MS` | 3 min | meter drift while awake |
| `SLEEP_GAIN_MS` | 15 s | +1 energy while asleep |

### Score player

`seq_add(delay, instrument, pitch, velocity, duration)` queues up to 24 timed notes; `seq_tick()` posts them to the engine when due. Chimes, fanfares, riff playback and the breathing bass use it.

## 4. Drawing

- **One canvas** of 240 × 240 RGB565 in the `.pool` RAM region (`gfx.c`, 115,200 bytes). `render()` clears it, draws the screen and sends it to the ST7789 panel in **two transfers of 240 × 120** (57,600 bytes each).
- The screen is redrawn only when something changed (`ui.redraw`) or an animation frame is due on an animated screen.
- **Page model.** Most screens are a `page_t`: header (left/right), optional art, lines, options, footer hint, plus a reserved `extra_h` for custom drawing (bars, slots, the mood scale). `draw_page()` centres the content vertically between the header line (y 21) and the footer line (y 218).
- **Fonts** come from `tools/gen_font.py` (Terminus): `FONT_XS` 12 px for hints, `FONT_B` 16 px bold for text (26 characters fit), `FONT_M` 24 px, `FONT_L` 32 px for the sober-day total.
- **Pixel art** (`sg_art.h`): a 32 × 26 grid stored as row bitmasks (`px_rows`). Bitmaps are strings of `#` and `.`. Scenes are composed with `px_bm()`, `px_sprite()`, `px_stage_scene()`, `px_wave()` and drawn by `px_draw(x, y, size, ink)` as squares with a 1-pixel gap at sizes above 3, for the LCD-dot look.
- **Colours:** `LCD_BG` RGB(169,189,139), `LCD_INK` RGB(30,43,22), `LCD_DIM`. The stage-up screen swaps ink and paper.

## 5. Sound (`dsp/omni.c`)

Eight voices; each voice is one of four instruments:

| Instrument | Used for | Synthesis |
| --- | --- | --- |
| `INS_BASS` | Low's keyboard, the SOS breath, fanfare root | Two-operator FM, modulator ratio 1, index 2.4 decaying to 0.7; 5 ms attack, ~120 ms release; an octave below the keyboard |
| `INS_DRUMS` | Kit | Kick (sine sweep ~150 Hz down), snare (noise + 190 Hz), hat (high-passed noise), clap (three bursts and a tail) |
| `INS_PLUCK` | Vinyl | FM, ratio 2, fast-decaying index and amplitude |
| `INS_BELL` | UI chimes, SOS play-along | FM, ratio 3.5, slow decay |

The keyboard plays **A minor pentatonic** from C3 to C6 on the 16 white keys (`om_white_index()`); on Kit the white keys cycle kick, snare, hat, clap. `om_note()` plays any instrument for a set duration. The mix is soft-limited (`x / (1 + |x|)`) and scaled by the LEVEL setting and the MASTER knob. No samples are stored.

API (`omni.h`): `om_init`, `om_instrument`, `om_trigger`, `om_note_off`, `om_note`, `om_set(P_LEVEL)`, `om_panic`, `om_render`.

## 6. Game state (`sg_state.c`)

`sg_t sg` holds everything saved plus session timers.

- **Days:** `sg_can_declare()` returns 0 (already counted), 1 (count) or 2 (ask "a new day?", after a restart). `sg_declare()` raises `total_days`, `streak`, `best_streak` and mood.
- **Slip:** `sg_slip()` sets `streak = 0` and counts the slip. Nothing else changes.
- **Stages:** `sg_stage_for(days, lessons)` walks the thresholds `{0,3,14,30,90}` days and `{0,3,10,20,40}` lessons; `sg_update_stage()` only ever raises `sg.stage`.
- **Meters:** energy, mood and rhythm stay between 10 and 100 (`SG_METER_FLOOR`).
- **Riffs:** up to 48 riffs of up to 4 notes; `sg_add_riff()` also counts the lesson.

## 7. Save format (`project.c`)

One storage object (`OBJ_PROJ`), written by `storage.c` with A/B sectors and CRC-32, so an interrupted write keeps the last good copy. All values little-endian.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | Magic `SGRV` |
| 4 | 1 | Version (1) |
| 5 | 1 | 0 |
| 6 | 1 | Volume (LEVEL, 0–100) |
| 7 | 1 | 0 |
| 8 | 1 | Buddy (0 Low, 1 Kit, 2 Vinyl) |
| 9 | 1 | Onboarded (0/1) |
| 10 | 1 | Thoughts on (0/1) |
| 11 | 1 | Sleeping (0/1) |
| 12 | 3 | Energy, mood, rhythm (10–100) |
| 15 | 1 | Stage (1–5) |
| 16 | 2 | Total sober days |
| 18 | 2 | Streak |
| 20 | 2 | Best streak |
| 22 | 2 | Waves ridden |
| 24 | 2 | Lessons |
| 26 | 2 | Slips |
| 28 | 2 | Sober sessions (reserved for Gig/Session mode) |
| 30 | 1 | Next thought index |
| 31 | 1 | 0 |
| 32 | 2 | Day of the last thought (0xFFFF = none) |
| 34 | 2 | Day of the last professional mention (0xFFFF = none) |
| 36 | 1 | Number of riffs (0–48) |
| 37 | 1 | 0 |
| 38 | 2 | Total length in bytes |
| 40 | 7 × n | Riffs: day (2), note count 1–4 (1), notes 0–15 (4) |

Maximum size: 40 + 48 × 7 = 376 bytes (an object holds 3,840). `sg_valid()` rejects anything malformed; Sober Groove then starts fresh. On load the stage is recomputed from the counters, so it can only rise. **Change `SG_SAVE_VERSION` whenever the layout changes**, and add a migration from the previous version.

### Autosave

`autosave()` saves when no key is held:

- **important** changes (a day, a wave, a lesson, a slip, a setting, finishing the introduction): 0.8 s after the change;
- other changes (care, meters): 5 s after the change and at least 60 s after the last save;
- `sg_needs_save()` (which packs and compares) runs at most once a second.

## 8. Tests and the simulator

```sh
X0X_FONTSET=terminus sh tests/run_tests.sh
```

builds `build/host/omni_host` and runs the scenarios in `tests/sg/`, then the storage test.

| Scenario | Covers |
| --- | --- |
| `first_run.omni` | Splash, introduction, choosing Kit, prologue, first morning, care, status, "already counted", a restart and "a new day?" |
| `sos.omni` | A full 20-minute wave, HALT, a tip, help during the wave, the end, an early exit |
| `lesson_slip.omni` | Three days, three lessons, stage up to STREET, repertoire playback, the slip flow, settings, persistence |
| `audio.omni` | Silence at rest; each buddy's instrument and the SOS bass audible and below clipping |

Script commands are documented at the top of `host/omni_host.c`. `shot FILE.png` saves the screen, `wav FILE` records audio, `skip MS` advances time fast without audio, `reboot` restarts from the simulated flash. `expect NAME VALUE` checks state (`<N` and `>N` are bounds).

## 9. Build checks

`tools/build.py` (via `build.sh`) compiles with the JieLi clang toolchain and refuses to package when:

- `_start` or the entry stub is not where the bootloader jumps;
- `.data + .bss` exceed 98,304 bytes or the `.pool` region overflows (the 240 × 240 canvas uses 115,200 of 344,064 bytes);
- RAM-only flash routines call into flash;
- any file outside `firmware/hal/` touches hardware registers, casts volatile pointers or uses inline assembly (`mmio_check()`).

`mmio_check()` can be run on its own without the toolchain:

```sh
python3 -c "import sys; sys.path.insert(0, 'tools'); import build; print(build.mmio_check())"
```

## 10. Extending

- **A new sentence:** edit `sg_text.h`. Keep lines at 26 characters or fewer in `FONT_B` (about 208 pixels) and follow [CONTENT-GUIDE.md](CONTENT-GUIDE.md).
- **A new thought:** add a `thought_t` to `THOUGHTS[]` with its kind. The rotation and the once-a-week limit for `TH_PRO` apply automatically.
- **A new screen:** add it to the enum (append, so the test numbers stay), draw it in `render()`, handle `SEL`/`GLO` in `on_sel()`/`on_glo()` and its option count in `opt_count()`.
- **A new lesson type:** add a state for recording and playback in `ui.c`, store the result as an `sg_riff_t` (or extend the save format) and call `sg_add_riff()` so it counts.
- **A new stage prop:** add a bitmap to `sg_art.h` and place it in `px_stage_scene()`.
- **A new instrument:** add an `INS_*` to `omni.h`, its voice code to `voice_sample()` and its start values to `start()`.

Add or extend a scenario in `tests/sg/` for every behaviour you change.
