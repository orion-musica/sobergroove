// SPDX-License-Identifier: GPL-3.0-only
// The web installer's engine (docs/install/fm1web.js) against a simulated FM-1 over fake WebMIDI:
// 3310 (FM1 Open) -> Sober Groove, stock V15 -> Sober Groove, Sober Groove -> V15, and a resume.
//   node tests/web/installer_test.mjs PACKAGE.fwsc
import fs from "node:fs";
import crypto from "node:crypto";
import path from "node:path";
const dir = path.resolve(path.dirname(new URL(import.meta.url).pathname), "../../docs/install");
globalThis.location = { href: "http://localhost/install/", origin: "http://localhost" };
globalThis.fetch = async (url) => {
  const u = new URL(url); const data = fs.readFileSync(path.join(dir, path.basename(u.pathname)));
  return { ok: true, headers: { get: () => String(data.length) }, body: { getReader() { let d = false; return {
    read: async () => (d ? { done: true } : ((d = true), { done: false, value: new Uint8Array(data) })), cancel: async () => {} }; } } };
};
eval(fs.readFileSync(path.join(dir, "fm1web.js"), "utf8"));
const W = globalThis.FM1Web;
const pkgFile = process.argv[2] || path.join(dir, "sobergroove.fwsc");
const pkgBytes = fs.readFileSync(pkgFile);
const sha = (b) => crypto.createHash("sha256").update(b).digest("hex");

function unframe(bytes) { return W.parse(Uint8Array.from(bytes)); }
const reply = (cmd, data) => { // device -> host frame, same format as frame()
  let s = 0; for (const b of data) s = (s + b) & 255;
  return Uint8Array.from([240, ...W.pack7([0, 89, cmd, data.length & 255, (data.length >> 8) & 255, 0, ...data, (~s) & 255]), 247]);
};
const ident = (s) => { const a = new Array(27).fill(0); [...s].forEach((c, i) => (a[i] = c.charCodeAt(0))); return a; };

class FakeFM1 {           // what the firmware on the device does, by identity
  constructor(identity) { this.identity = identity; this.armed = false; this.events = []; this.make(); }
  make() {
    const dev = this;
    this.input = { name: /^ota-/.test(this.identity) ? "FM-1 Update" : "FM-1", manufacturer: "", state: "connected", l: [],
      addEventListener(_, f) { this.l.push(f); }, removeEventListener(_, f) { this.l = this.l.filter((x) => x !== f); },
      open: async () => {}, close: async () => {} };
    this.output = { name: this.input.name, manufacturer: "", state: "connected", open: async () => {}, close: async () => {},
      send: (b) => setTimeout(() => dev.recv(b), 1) };
  }
  emit(b) { for (const f of this.input.l) f({ data: b }); }
  reboot(identity, ms = 30) {
    this.input.state = this.output.state = "disconnected"; this.events.push("boot " + identity);
    setTimeout(() => { this.identity = identity; this.armed = false; this.make(); access.plug(this); }, ms);
  }
  recv(b) {
    if (this.input.state !== "connected") return;
    if (b.length === 6 && b[1] === 0x22) return this.upgrade();
    const p = unframe(b); if (!p) return;
    if (p.command === 17) return this.emit(reply(17, ident(this.identity)));
    const open = /^FM-1_8\d{6}$/.test(this.identity);
    if (p.command === 49 && open) {           // FM1 Open capability
      const c = new Array(47).fill(0); "FMC1".split("").forEach((x, i) => (c[i] = x.charCodeAt(0)));
      c[4] = 1; c[5] = 7; c[6] = 0; c[7] = 1; c[8] = 0; c[9] = 2; c[10] = 0; c[11] = 0; c[12] = 16; c[13] = 0;
      return this.emit(reply(49, c));
    }
    if (p.command === 50 && open) { this.armed = true; this.events.push("armed"); return this.emit(reply(50, [0])); }
    if (p.command === 48) return this.onData(p.data);
  }
  upgrade() {
    const open = /^FM-1_8\d{6}$/.test(this.identity), ota = /^ota-/.test(this.identity);
    if (open && !this.armed) { this.events.push("UPGRADE ignored (not armed)"); return; }   // what broke the first installer
    this.events.push("upgrade " + this.identity);
    this.phase = ota ? "write" : "stage"; this.at = 0; this.ask();
  }
  ask() {
    const end = this.phase === "write" ? 609000 : 8192, finish = this.phase === "write" ? 0xf0000000 : 0xe0000000;
    const addr = this.at >= end ? finish : this.at, len = this.at >= end ? 8 : 512;
    this.pending = addr;
    this.emit(reply(48, [7, addr & 255, (addr >>> 8) & 255, (addr >>> 16) & 255, addr >>> 24, len & 255, (len >> 8) & 255, 0]));
  }
  onData(d) {
    const addr = d[1] | (d[2] << 8) | (d[3] << 16) | (d[4] << 24) >>> 0;
    if (this.pending >= 0xe0000000) {
      if (this.phase === "stage") this.reboot("ota-FM-1_015");
      else this.reboot(this.target);
      this.pending = -1; return;
    }
    this.at += 512; if (this.abortAt && this.at >= this.abortAt) { this.abortAt = 0; this.events.push("unplugged"); this.input.state = this.output.state = "disconnected"; return; }
    setTimeout(() => this.ask(), 0);
  }
}
const access = {
  inputs: new Map(), outputs: new Map(), dev: null,
  plug(d) { this.dev = d; this.inputs = new Map([["i", d.input]]); this.outputs = new Map([["o", d.output]]); },
};

async function run(name, startId, which, target, opts = {}) {
  const dev = new FakeFM1(startId); dev.target = target; access.plug(dev);
  if (opts.abortAt) dev.abortAt = opts.abortAt;
  const pkg = await W.prepare(meta[which]);
  const u = new W.Updater(access, pkg, () => {}, { idle: 2000, writeIdle: 4000, reconnect: 3000, settle: 50 });
  const c = await u.connect({ force: true });
  let err = null;
  try { await u.install({ force: true }); } catch (e) { err = e.message; }
  console.log(`${err ? "FAIL" : "ok  "} ${name}: connect ${JSON.stringify(c)} -> ${access.dev.identity}${err ? " (" + err + ")" : ""}\n       ${dev.events.join(" | ")}`);
  return { err, dev: access.dev };
}

const page = fs.readFileSync(path.join(dir, "index.html"), "utf8");
const meta = JSON.parse(page.match(/const META = (\{.*?\});\n/s)[1]);
if (process.argv[2]) Object.assign(meta.sg, { url: "x", sha256: sha(pkgBytes), build_id: sha(pkgBytes), bytes: pkgBytes.length });
if (process.argv[2]) globalThis.fetch = (orig => async (u) => (String(u).endsWith("/x") ? { ok: true, headers: { get: () => String(pkgBytes.length) },
  body: { getReader() { let d = false; return { read: async () => (d ? { done: true } : ((d = true), { done: false, value: new Uint8Array(pkgBytes) })), cancel: async () => {} }; } } } : orig(u)))(globalThis.fetch);

let bad = 0;
bad += !!(await run("3310 -> Sober Groove", "FM-1_8123456", "sg", "FM-1_506370001")).err;
bad += !!(await run("stock V15 -> Sober Groove", "FM-1_015", "sg", "FM-1_506370001")).err;
bad += !!(await run("Sober Groove -> Sober Groove", "FM-1_506370001", "sg", "FM-1_506370001")).err;
bad += !!(await run("update mode -> Sober Groove (resume)", "ota-FM-1_015", "sg", "FM-1_506370001")).err;
process.exit(bad ? 1 : 0);
