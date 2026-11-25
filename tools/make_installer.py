#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the web installer page: docs/install/index.html (served by GitHub Pages).

  tools/make_installer.py [PACKAGE.fwsc VERSION]

Inlines web/fm1pkg.js and web/fm1ota.js into web/installer.html. With a package, also copies
it to docs/install/sobergroove.fwsc after checking its identity; the page loads it from there.
Without one, the page offers a file picker until a package is published."""
import json
import re
import shutil
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
OUT = SRC / "docs" / "install"
PRODUCT = "FM-1_506370001"
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    return "".join(chr((raw[i * BLK + KEEP] - i - 1) & 0xFF) for i in range(BLOCKS) if raw[i * BLK + KEEP] != 0x7D)


def main(argv):
    version = "0.1.0-alpha"
    OUT.mkdir(parents=True, exist_ok=True)
    if len(argv) == 3:
        pkg, version = Path(argv[1]), argv[2]
        raw = pkg.read_bytes()
        if product_of(raw) != PRODUCT:
            raise SystemExit(f"{pkg}: identity {product_of(raw)!r}, expected {PRODUCT}")
        shutil.copy(pkg, OUT / "sobergroove.fwsc")
    elif len(argv) != 1:
        raise SystemExit(__doc__)
    page = (SRC / "web" / "installer.html").read_text(encoding="utf-8")
    lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": version, "product": PRODUCT, "pkg": "sobergroove.fwsc"})
    for mark in ("/*LIB*/", "/*META*/"):
        if page.count(mark) != 1:
            raise SystemExit(f"installer.html must contain {mark} once")
    (OUT / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    print(f"installer: {OUT / 'index.html'} (version {version})")


if __name__ == "__main__":
    main(sys.argv)
