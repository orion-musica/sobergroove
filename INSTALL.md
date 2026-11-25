# Installation

Alpha 0.1.0 has no prebuilt package: build `build/sobergroove-0.1.0-alpha.fwsc` first ([BUILDING.md](BUILDING.md)). The installer needs Python 3, mido and python-rtmidi, pinned in `requirements-install.txt`.

After activating the installation environment, connect the FM-1 and run:

```sh
python tools/fm1_install.py --info
python tools/fm1_install.py build/sobergroove-0.1.0-alpha.fwsc
```

The first command only reads the device identity. The second asks for confirmation before installing. Keep USB connected until the write and restart finish. Expected identity: **FM-1_506370001**.

To go back, install the M-VAVE firmware or another community firmware the same way. An interrupted update is finished by running the same command again; `tools/fm1_rescue.py` helps with a device stuck in update mode.

## Easiest: install in the browser

Open **https://orion-musica.github.io/sobergroove/install/** in Chrome or Edge on a computer, connect the FM-1 with a USB data cable, press **Connect FM-1**, allow MIDI and SysEx, tick the backup box and press **Install Sober Groove**. Do not unplug until it says DONE. The page uses the same update engine as 3310 (FM1 Open's browser updater) and has M-VAVE's original V15 built in, so you can always go back.

Coming from 3310 or another FM1 Open firmware: the safest route is two steps on the same page, first **Original M-VAVE V15**, then **Sober Groove**.

The page serves the package built by the latest GitHub Actions run. If it says no package is published yet, download `sobergroove-*.fwsc` from the release and choose it with the file picker on the page.

