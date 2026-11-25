# Contributing to Sober Groove

Thank you for helping. Sober Groove is for people in a fragile moment of their lives, so care counts as much as code.

## Ways to help

- **Test on hardware.** The alpha needs its first device builds. Follow [docs/HARDWARE-TESTING.md](docs/HARDWARE-TESTING.md) and open an issue with the results.
- **Review wording.** If you work in addiction support, read `firmware/src/app/sg_text.h` against [docs/CONTENT-GUIDE.md](docs/CONTENT-GUIDE.md).
- **Code:** lesson types for Kit and Vinyl, Gig mode, the web companion, Polish text. See [RELEASE-NOTES.md](RELEASE-NOTES.md) for what is planned.
- **Art:** new stage props and animations in the 32 × 26 LCD grid (`sg_art.h`). Original art only; no known characters.

## Before a pull request

1. Read [docs/DESIGN.md](docs/DESIGN.md). Every change must pass its seven rules: no death, a slip is not a relapse, no judgement, no alcohol imagery, people before the device, small wins count, privacy.
2. Run the tests: `X0X_FONTSET=terminus sh tests/run_tests.sh`.
3. Add or extend a scenario in `tests/sg/` for behaviour you change. Attach simulator screenshots for visual changes.
4. If you change the save layout, raise `SG_SAVE_VERSION` and add a migration ([docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#7-save-format-projectc)).
5. Keep hardware access inside `firmware/hal/`. The build refuses anything else.
6. Wording changes: say why, list sources for facts, follow the content guide.

## Style

C99, the style of the surrounding code: short functions, no dynamic memory, no floating point outside `dsp/`, static buffers sized at compile time, nothing blocking in the audio interrupt.

## Licence

By contributing you agree that your contribution is licensed under GPL-3.0-only, like the rest of the project.

## Conduct

Be kind. Many people here have their own story with alcohol. Never shame anyone for theirs.
