# Build Sober Groove

Source builds need Python 3, a C compiler and Git. The target compiler is JieLi's Linux x86-64 toolchain; other hosts need Docker running.

## Firmware package

Run from the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-build.txt
sh tools/get_toolchain.sh "$PWD/tooling/jieli"
git clone --branch AC79NN_SDK_V1.2.1_2023-12-13 --depth 1 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK tooling/ac79-sdk
export JIELI_TOOLCHAIN="$PWD/tooling/jieli/toolchain"
export AC79_SDK="$PWD/tooling/ac79-sdk"
export PYTHON="$PWD/.venv/bin/python"
export X0X_FONTSET=terminus
sh tests/run_tests.sh
sh build.sh --release 0.1.0-alpha
```

Output: `build/sobergroove-0.1.0-alpha.fwsc`, identity **FM-1_506370001**, plus `build/omni.bin` and `build/loader/ota.bin`. The build checks the SDK files, the image, RAM and pool use, RAM-only flash routines and that hardware registers are touched only in `firmware/hal/`. Builds and tests never access hardware. Test a package on a device before sharing it.

The upstream tested compiler was jieli-linux-toolchains-20260730.1 with SDK commit d179b4484759423312073f5fbb232501aa491047.

## Simulator and tests

`sh host/build_host.sh` builds `build/host/omni_host`, the whole app (UI, game state, sound, drawing, saving) against a simulated FM-1 driven by a script. It writes screenshots (PNG) and audio (WAV). `sh tests/run_tests.sh` builds it and runs every scenario in `tests/sg/`.

Script commands are listed at the top of `host/omni_host.c`. Sober Groove adds `skip MS` (advance time fast without audio, for a 20-minute wave) and these `expect` names: `screen days streak best waves lessons riffs slips stage character onboarded thoughts_on energy mood rhythm sleeping sos notes`.

## Where things are

| Path | What |
| --- | --- |
| `firmware/src/app/ui.c` | Screens, input, SOS, lessons, the menu, autosave |
| `firmware/src/app/sg_state.c` | Days, streak, waves, lessons, stages, repertoire |
| `firmware/src/app/project.c` | The save format (one storage object, A/B sectors and CRC from `storage.c`) |
| `firmware/src/app/sg_text.h` | Every sentence on screen: thoughts, prologue, help, tips |
| `firmware/src/app/sg_art.h` | LCD-style pixel art: buddies, stages, props |
| `firmware/src/dsp/omni.c` | Sound: FM bass, synthesized drums, pluck, bell |
| `firmware/hal`, `firmware/loader`, `firmware/src/*.c`, `tools/` | The FM-1 platform, unchanged from upstream |

Wording lives in `sg_text.h` so it can be reviewed by an addiction professional without touching the logic.
