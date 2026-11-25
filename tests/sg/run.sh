#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Sober Groove scenarios in the host simulator: no device, no network.
set -eu
cd "$(dirname "$0")/../.."
for t in first_run sos lesson_slip audio; do
    mkdir -p "build/test/$t"
    echo "== $t"
    build/host/omni_host "tests/sg/$t.omni" "build/test/$t" | grep -v "^  ok"
done
cc -O2 tests/storage_test.c -o build/test/storage_test
build/test/storage_test
