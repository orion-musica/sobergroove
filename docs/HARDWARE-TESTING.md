# Hardware test checklist

The alpha runs in the simulator but has not been built with the JieLi toolchain or installed on an FM-1 yet. Use this checklist for the first device builds and report results in an issue titled `Hardware test: <your FM-1 / starting firmware>`.

## Before you start

- Note the starting firmware (`python tools/fm1_install.py --info`) so you can go back.
- Have the M-VAVE firmware or the firmware you came from ready to reinstall.
- Use a USB data cable and close any music software using MIDI.

## Build

- [ ] `sh tests/run_tests.sh` passes.
- [ ] `sh build.sh --release 0.1.0-alpha` passes every check. Paste the `ok` lines (image size, RAM, pool) in the report.
- [ ] `build/sobergroove-0.1.0-alpha.fwsc` exists.

## Install

- [ ] The installer finds the FM-1 and writes the package.
- [ ] After restart, `--info` reports **FM-1_506370001**.

## First start

- [ ] The splash appears within a few seconds, readable, colours correct (green LCD background, dark ink).
- [ ] No tearing or half-drawn screens when screens change (the screen is sent in two halves).
- [ ] SELECT and SEL work through the introduction; the knob direction feels right.
- [ ] Choosing each buddy shows the right sprite and the white keys play the right sound (bass, drums, pluck).

## Daily use

- [ ] Holding HOME for 1.5 s counts a day; a tap opens status.
- [ ] After a power cycle the "a new day?" question appears.
- [ ] Black keys 1–4 do meal, talk, move, sleep; the keys are lit.
- [ ] Lit keys and buttons match each screen.

## SOS

- [ ] Black key 5 starts SOS from home, status, the menu and a lesson.
- [ ] The bass breath sounds every 10 s; white keys play the bell.
- [ ] The HALT check appears at 5 minutes; black keys 1–4 answer.
- [ ] Holding key 5 for 2 s asks "Feeling lighter?".
- [ ] A full 20-minute wave ends with "Wave passed" and a fanfare.
- [ ] Audio has no clicks or dropouts while the screen redraws during a wave.

## Lessons

- [ ] Four notes recorded; the buddy plays them back in time.
- [ ] "Teach again" and "Keep it like this" both work; the repertoire plays riffs back.

## Saving

- [ ] SAVE shows "Saved."
- [ ] Power off right after a new day, a lesson and a slip: everything is there after power on.
- [ ] Power off during a save does not lose the previous state.

## Power and performance

- [ ] The UI stays responsive while sound plays (hold several keys, start a fanfare).
- [ ] Report anything slow, any freeze or any reboot (the platform's crash counter is in `om_dbg`).

## Going back

- [ ] Reinstalling the starting firmware works.
