# Installation

Alpha 0.1.0 has no prebuilt package: build `build/sobergroove-0.1.0-alpha.fwsc` first ([BUILDING.md](BUILDING.md)). The installer needs Python 3, mido and python-rtmidi, pinned in `requirements-install.txt`.

After activating the installation environment, connect the FM-1 and run:

```sh
python tools/fm1_install.py --info
python tools/fm1_install.py build/sobergroove-0.1.0-alpha.fwsc
```

The first command only reads the device identity. The second asks for confirmation before installing. Keep USB connected until the write and restart finish. Expected identity: **FM-1_506370001**.

To go back, install the M-VAVE firmware or another community firmware the same way. An interrupted update is finished by running the same command again; `tools/fm1_rescue.py` helps with a device stuck in update mode.
