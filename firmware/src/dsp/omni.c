/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove sound: two-operator FM bass, synthesized drums, a plucked stab and a soft bell.
 * Single UI producer, audio-IRQ consumer; only render owns live DSP state.
 * The command ring and release generations follow Trumpagotchi's engine (GPL-3.0-only). */
#include "omni.h"
#include "fastmath.h"

static const om_param_t params[P_NPARAMS] = { {"LEVEL", 0, 100, 80} };
static const int8_t white_index[FMO_KEYS] = {
    0,-1,1,-1,2,-1,3,4,-1,5,-1,6,7,-1,8,-1,9,-1,10,11,-1,12,-1,13,14,-1,15
};
/* A minor pentatonic over C3 .. C6 (the same notes as C major pentatonic). */
static const float frequencies[16] = {
    130.81278f,146.83238f,164.81378f,195.99772f,220.0f,
    261.62557f,293.66477f,329.62756f,391.99544f,440.0f,
    523.25113f,587.32954f,659.25511f,783.99087f,880.0f,1046.50226f
};
static const char *const note_names[16] = {
    "C3","D3","E3","G3","A3","C4","D4","E4","G4","A4","C5","D5","E5","G5","A5","C6"
};
int om_white_index(int k) { return k >= 0 && k < FMO_KEYS ? white_index[k] : -1; }
static int white(int k) { return om_white_index(k) >= 0; }
const om_param_t *om_param_info(int p) { return p >= 0 && p < P_NPARAMS ? &params[p] : 0; }
const char *om_sample_name(int k) { return white(k) ? note_names[white_index[k]] : ""; }
volatile uint32_t om_active_keys, om_triggers, om_notes, om_dropped;

#define QN 128u
#define SR 44100.0f
#define BARRIER() __asm__ volatile("" ::: "memory")
enum { C_KEY, C_SET, C_INS, C_NOTE };
typedef struct { int type, a, b, c, d; uint32_t release; } command_t;
static command_t queue[QN];
static volatile uint32_t qw, qr, panic_pending;
/* Release generations cannot be dropped by a full queue: a queued old note-on cannot
 * resurrect a released key. Each word has one writer (UI); the ISR only reads them. */
static volatile uint32_t released[FMO_KEYS];
static int par[P_NPARAMS];
static int keyboard_ins;

typedef struct {
    int active, gated, key, ins, pitch;
    uint32_t age, release, remaining, t;    /* remaining: samples until a timed note releases (0 = held) */
    float phase, mphase, step, gain, env, index, tone, noise_lp;
} voice_t;
static voice_t voices[FMO_VOICES];
static uint32_t age, noise_state = 0x6b43a9b5u;

static int post(int type, int a, int b, int c, int d)
{
    uint32_t w = qw;
    if (w - qr >= QN) { om_dropped++; return 0; }
    queue[w % QN] = (command_t){type, a, b, c, d, type == C_KEY ? released[a] : 0u};
    BARRIER(); qw = w + 1u;
    return 1;
}
void om_set(int p, int value) { if (p >= 0 && p < P_NPARAMS) post(C_SET, p, value, 0, 0); }
void om_instrument(int ins) { if (ins >= 0 && ins < INS_NINS) post(C_INS, ins, 0, 0, 0); }
void om_trigger(int key, int velocity)
{
    if (white(key) && velocity > 0) post(C_KEY, key, velocity > 127 ? 127 : velocity, 0, 0);
}
void om_note_off(int key) { if (white(key)) { released[key]++; BARRIER(); } }
void om_note(int ins, int pitch, int velocity, int dur_ms)
{
    if (ins < 0 || ins >= INS_NINS || velocity <= 0) return;
    if (dur_ms < 10) dur_ms = 10;
    if (dur_ms > 8000) dur_ms = 8000;
    post(C_NOTE, ins, pitch, velocity > 127 ? 127 : velocity, dur_ms);
}
void om_panic(void) { panic_pending = 1; }

static float noise(void)
{
    noise_state = noise_state * 1664525u + 1013904223u;
    return (float)(noise_state >> 8) * (1.0f / 8388608.0f) - 1.0f;
}
static float pitch_hz(int ins, int pitch)
{
    float hz = frequencies[(pitch < 0 ? 0 : pitch > 15 ? 15 : pitch)];
    if (ins == INS_BASS) return hz * 0.5f;     /* C2 .. C5 */
    return hz;
}
static void start(int ins, int pitch, int vel, int key, uint32_t release, uint32_t remaining)
{
    uint32_t j; int slot = -1;
    if (key >= 0)
        for (j = 0; j < FMO_VOICES; j++) if (voices[j].active && voices[j].key == key) { slot = (int)j; break; }
    if (slot < 0) for (j = 0; j < FMO_VOICES; j++) if (!voices[j].active) { slot = (int)j; break; }
    if (slot < 0) {
        slot = 0;
        for (j = 1; j < FMO_VOICES; j++)
            if ((uint32_t)(age - voices[j].age) > (uint32_t)(age - voices[slot].age)) slot = (int)j;
    }
    {
        voice_t *v = &voices[slot];
        int drum = ins == INS_DRUMS ? (pitch & 3) : 0;
        v->active = 1; v->gated = 1; v->key = key; v->ins = ins; v->pitch = drum;
        v->age = ++age; v->release = release; v->remaining = remaining;
        v->phase = v->mphase = 0; v->noise_lp = 0;
        v->gain = (float)vel / 127.0f;
        v->env = ins == INS_BASS ? 0.0f : 1.0f;
        v->t = 0;
        if (ins == INS_DRUMS) {
            v->step = (drum == DR_KICK ? 150.0f : drum == DR_SNARE ? 190.0f : 0.0f) / SR;
            v->tone = drum == DR_KICK ? 1.0f : 0.0f;          /* kick: pitch sweep amount */
            v->index = 0;
        } else {
            v->step = pitch_hz(ins, pitch) / SR;
            v->index = ins == INS_BASS ? 2.4f : ins == INS_PLUCK ? 3.2f : 1.6f;
            v->tone = 0;
        }
    }
    om_notes++;
}
static void clear_sound(void)
{
    uint32_t i;
    for (i = 0; i < FMO_VOICES; i++) voices[i] = (voice_t){0};
    om_active_keys = 0;
}
void om_init(void)
{
    int i;
    qw = qr = panic_pending = 0; age = om_triggers = om_notes = om_dropped = 0;
    for (i = 0; i < FMO_KEYS; i++) released[i] = 0;
    for (i = 0; i < P_NPARAMS; i++) par[i] = params[i].def;
    keyboard_ins = INS_BASS;
    clear_sound();
}
static float limit(float x) { return x / (1.0f + fm_fabsf(x)); }

/* one voice, one sample; returns 0 and deactivates it when silent */
static float voice_sample(voice_t *v)
{
    float s = 0;
    if (v->remaining) { if (--v->remaining == 0) v->gated = 0; }
    switch (v->ins) {
    case INS_BASS: {
        float m;
        if (v->gated) { v->env += 1.0f / 220.0f; if (v->env > 1.0f) v->env = 1.0f; }
        else { v->env -= 1.0f / 5300.0f; if (v->env <= 0) { v->active = 0; return 0; } }
        v->index = 0.7f + (v->index - 0.7f) * 0.99985f;
        v->mphase += v->step; if (v->mphase >= 1.0f) v->mphase -= 1.0f;
        m = fm_sin_turns(v->mphase) * v->index * 0.16f;
        v->phase += v->step; if (v->phase >= 1.0f) v->phase -= 1.0f;
        {
            float p = v->phase + m;
            p -= fm_floorf(p);
            s = fm_sin_turns(p) * 0.55f;
        }
        break;
    }
    case INS_PLUCK:
    case INS_BELL: {
        float m, ratio = v->ins == INS_PLUCK ? 2.0f : 3.5f;
        float decay = v->ins == INS_PLUCK ? 0.99984f : 0.99993f;
        if (!v->gated) decay = v->ins == INS_PLUCK ? 0.9993f : 0.9997f;
        v->env *= decay;
        if (v->env < 0.0005f) { v->active = 0; return 0; }
        v->index *= v->ins == INS_PLUCK ? 0.9996f : 0.99985f;
        v->mphase += v->step * ratio; if (v->mphase >= 1.0f) v->mphase -= fm_floorf(v->mphase);
        m = fm_sin_turns(v->mphase) * v->index * 0.16f;
        v->phase += v->step; if (v->phase >= 1.0f) v->phase -= 1.0f;
        {
            float p = v->phase + m;
            p -= fm_floorf(p);
            s = fm_sin_turns(p) * (v->ins == INS_PLUCK ? 0.45f : 0.32f);
        }
        break;
    }
    case INS_DRUMS:
    default: {
        float n;
        switch (v->pitch) {
        case DR_KICK:
            v->phase += v->step * (1.0f + 2.2f * v->tone);
            if (v->phase >= 1.0f) v->phase -= 1.0f;
            v->tone *= 0.9992f;
            s = fm_sin_turns(v->phase) * 0.9f;
            v->env *= 0.99975f;
            break;
        case DR_SNARE:
            n = noise();
            v->phase += v->step; if (v->phase >= 1.0f) v->phase -= 1.0f;
            s = 0.55f * n + 0.35f * fm_sin_turns(v->phase);
            v->env *= 0.9996f;
            break;
        case DR_HAT:
            n = noise();
            v->noise_lp += 0.35f * (n - v->noise_lp);
            s = (n - v->noise_lp) * 0.6f;
            v->env *= 0.9985f;
            break;
        default: {                                  /* clap: three quick bursts, then a tail */
            uint32_t t = v->t;
            float burst = 1.0f;
            n = noise();
            v->noise_lp += 0.25f * (n - v->noise_lp);
            if (t < 1800u) burst = (t % 600u) < 300u ? 1.0f : 0.25f;
            v->t = t + 1u;
            s = (n - 0.5f * v->noise_lp) * 0.6f * burst;
            if (t >= 1800u) v->env *= 0.9993f;
            break;
        }
        }
        if (v->env < 0.0005f) { v->active = 0; return 0; }
        break;
    }
    }
    return s * v->env * v->gain;
}

void om_render(int32_t *out, uint32_t n, uint32_t gain_q12)
{
    uint32_t i, j, keys = 0;
    float master = (float)(gain_q12 > 4096 ? 4096 : gain_q12) * (1.0f / 4096.0f);
    float level = (float)par[P_LEVEL] * 0.01f;
    if (panic_pending) { qr = qw; panic_pending = 0; clear_sound(); }
    while (qr != qw) {
        command_t c; BARRIER(); c = queue[qr % QN]; BARRIER(); qr++;
        if (c.type == C_SET)
            par[c.a] = c.b < params[c.a].lo ? params[c.a].lo : c.b > params[c.a].hi ? params[c.a].hi : c.b;
        else if (c.type == C_INS)
            keyboard_ins = c.a;
        else if (c.type == C_NOTE)
            start(c.a, c.b, c.c, -1, 0, (uint32_t)c.d * 441u / 10u);
        else if (c.release == released[c.a]) {
            int wi = white_index[c.a];
            start(keyboard_ins, keyboard_ins == INS_DRUMS ? (wi & 3) : wi, c.b, c.a, c.release, 0);
            om_triggers++;
        }
    }
    for (j = 0; j < FMO_VOICES; j++)
        if (voices[j].active && voices[j].key >= 0 && voices[j].release != released[voices[j].key])
            voices[j].gated = 0;
    for (i = 0; i < n; i++) {
        float mix = 0, y;
        for (j = 0; j < FMO_VOICES; j++)
            if (voices[j].active) mix += voice_sample(&voices[j]);
        y = limit(mix * 1.2f) * master * level;
        out[2 * i] = (int32_t)(y * 8388607.0f);
        out[2 * i + 1] = (int32_t)(y * 8388607.0f);
    }
    for (j = 0; j < FMO_VOICES; j++)
        if (voices[j].active && voices[j].key >= 0) keys |= 1u << voices[j].key;
    om_active_keys = keys;
}
