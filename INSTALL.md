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

Open **https://orion-musica.github.io/sobergroove/install/** in Chrome or Edge on a computer, connect the FM-1 with a USB data cable, press **Install Sober Groove** and allow MIDI access. Do not unplug until it says Done. The same page can put M-VAVE's original firmware back.

The page serves the package built by the latest GitHub Actions run. If it says no package is published yet, download `sobergroove-*.fwsc` from the release and choose it with the file picker on the page.

