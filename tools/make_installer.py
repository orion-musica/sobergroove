#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the web installer: docs/install/ (served by GitHub Pages).

  tools/make_installer.py [PACKAGE.fwsc VERSION]

Copies the update engine (web/fm1web.js, web/fm1i18n.js), M-VAVE's original V15 and its notice,
and writes docs/install/index.html from web/installer.html with both packages' metadata.
With a package, it becomes docs/install/sobergroove.fwsc after an identity check; without one,
the page uses the package already there."""
import hashlib
import json
import shutil
import struct
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
OUT = SRC / "docs" / "install"
PRODUCT = "FM-1_506370001"
STOCK_SHA = "db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a"
STOCK_LOADER = "ef1f2db89b011fa53e71df363f0122de0ea896618c53fb53e8da676322004541"


def logical(raw):
    """identity and logical image of a .fwsc: one marker byte after each of the first 20 47-byte blocks"""
    img, product = bytearray(), ""
    for i in range(20):
        img += raw[i * 48:i * 48 + 47]
        m = raw[i * 48 + 47]
        if m != 0x7D:
            product += chr((m - i - 1) & 0xFF)
    return product, bytes(img + raw[960:])


def decode(b, key=0xFFFF):
    p = bytearray(b)
    for i in range(len(p)):
        p[i] ^= key & 0xFF
        key = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 0xFFFF
    return p


def loader_sha(img):
    head = decode(img[:64])
    count = struct.unpack_from("<H", head, 8)[0]
    for i in range(count):
        e = decode(img[64 + i * 80:144 + i * 80])
        typ, off, size = struct.unpack_from("<H", e, 0)[0], *struct.unpack_from("<II", e, 8)
        if typ == 100:
            return hashlib.sha256(img[off:off + size]).hexdigest()
    raise SystemExit("package has no update loader")


def main(argv):
    OUT.mkdir(parents=True, exist_ok=True)
    version = "0.1.0-alpha"
    if len(argv) == 3:
        shutil.copy(argv[1], OUT / "sobergroove.fwsc")
        version = argv[2]
    elif len(argv) != 1:
        raise SystemExit(__doc__)
    for name in ("fm1web.js", "fm1i18n.js", "FM-1_015.fwsc", "NOTICE.txt"):
        shutil.copy(SRC / "web" / name, OUT / name)
    stock = (OUT / "FM-1_015.fwsc").read_bytes()
    if hashlib.sha256(stock).hexdigest() != STOCK_SHA:
        raise SystemExit("web/FM-1_015.fwsc is not M-VAVE's original V15")
    meta = {"stock": {"schema": 1, "kind": "stock", "product": "FM-1_015", "family": 7, "build_id": STOCK_SHA,
                      "sha256": STOCK_SHA, "bytes": len(stock), "loader_sha256": STOCK_LOADER, "url": "FM-1_015.fwsc"}}
    pkg = OUT / "sobergroove.fwsc"
    if pkg.exists():
        raw = pkg.read_bytes()
        product, img = logical(raw)
        if product != PRODUCT:
            raise SystemExit(f"{pkg}: identity {product!r}, expected {PRODUCT}")
        sha = hashlib.sha256(raw).hexdigest()
        meta["sg"] = {"schema": 1, "kind": "foreign", "product": PRODUCT, "family": 7, "version": version,
                      "build_id": sha, "sha256": sha, "bytes": len(raw), "loader_sha256": loader_sha(img),
                      "url": f"sobergroove.fwsc?v={sha[:12]}"}
    else:
        meta["sg"] = {"schema": 1, "kind": "foreign", "product": PRODUCT, "family": 7, "version": version,
                      "build_id": "0" * 64, "sha256": "0" * 64, "bytes": 0, "loader_sha256": "0" * 64, "url": "sobergroove.fwsc"}
    page = (SRC / "web" / "installer.html").read_text(encoding="utf-8")
    if page.count("/*META*/") != 1:
        raise SystemExit("installer.html must contain /*META*/ once")
    (OUT / "index.html").write_text(page.replace("/*META*/", json.dumps(meta)), encoding="utf-8")
    print(f"installer: {OUT / 'index.html'} (Sober Groove {version}{'' if pkg.exists() else ', no package yet'})")


if __name__ == "__main__":
    main(sys.argv)
