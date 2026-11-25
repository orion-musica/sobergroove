/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove save: one storage object (A/B sectors and CRC come from storage.c).
 * Explicit little-endian layout, independent of C padding. Elapsed-time timers restart
 * at boot: time powered off never costs the buddy anything. */
#include "sg_state.c" /* application is built as one translation unit */
project_t proj;

#define SG_MAGIC0 'S'
#define SG_MAGIC1 'G'
#define SG_MAGIC2 'R'
#define SG_MAGIC3 'V'
#define SG_SAVE_VERSION 1u
#define SG_HEAD_BYTES 40u
#define SG_RIFF_BYTES (3u + SG_RIFF_NOTES)
#define SG_SAVE_MAX (SG_HEAD_BYTES + SG_MAX_RIFFS * SG_RIFF_BYTES)
_Static_assert(SG_SAVE_MAX <= PLAT_STORE_MAX, "save must fit one storage object");
_Static_assert(P_NPARAMS == 1, "change the save version when sound parameters change");

static uint8_t sg_saved[SG_SAVE_MAX];
static unsigned sg_saved_len;
static uint8_t sg_have_save;

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }

static unsigned sg_pack(uint8_t *out)
{
    unsigned i, j, n = SG_HEAD_BYTES;
    for (i = 0; i < SG_HEAD_BYTES; i++) out[i] = 0;
    out[0] = SG_MAGIC0; out[1] = SG_MAGIC1; out[2] = SG_MAGIC2; out[3] = SG_MAGIC3;
    out[4] = SG_SAVE_VERSION;
    out[6] = (uint8_t)proj.par[P_LEVEL];
    out[8] = sg.character; out[9] = sg.onboarded; out[10] = sg.thoughts_on; out[11] = sg.sleeping;
    out[12] = sg.energy; out[13] = sg.mood; out[14] = sg.rhythm; out[15] = sg.stage;
    put16(out + 16, sg.total_days); put16(out + 18, sg.streak); put16(out + 20, sg.best_streak);
    put16(out + 22, sg.waves); put16(out + 24, sg.lessons); put16(out + 26, sg.slips);
    put16(out + 28, sg.sober_sessions);
    out[30] = sg.thought_next;
    put16(out + 32, sg.last_thought_day); put16(out + 34, sg.last_pro_day);
    out[36] = sg.nriffs;
    for (i = 0; i < sg.nriffs; i++) {
        put16(out + n, sg.riffs[i].day); out[n + 2] = sg.riffs[i].n;
        for (j = 0; j < SG_RIFF_NOTES; j++) out[n + 3 + j] = sg.riffs[i].notes[j];
        n += SG_RIFF_BYTES;
    }
    put16(out + 38, (uint16_t)n);
    return n;
}

static int sg_valid(const uint8_t *in, int size)
{
    unsigned i, j, nr;
    if (size < (int)SG_HEAD_BYTES || size > (int)SG_SAVE_MAX) return 0;
    if (in[0] != SG_MAGIC0 || in[1] != SG_MAGIC1 || in[2] != SG_MAGIC2 || in[3] != SG_MAGIC3) return 0;
    if (in[4] != SG_SAVE_VERSION || get16(in + 38) != (uint16_t)size) return 0;
    if (in[6] > 100 || in[8] >= CH_N || in[9] > 1 || in[10] > 1 || in[11] > 1) return 0;
    for (i = 12; i < 15; i++) if (in[i] < SG_METER_FLOOR || in[i] > 100) return 0;
    if (in[15] < 1 || in[15] > SG_STAGES) return 0;
    nr = in[36];
    if (nr > SG_MAX_RIFFS || (unsigned)size != SG_HEAD_BYTES + nr * SG_RIFF_BYTES) return 0;
    for (i = 0; i < nr; i++) {
        const uint8_t *r = in + SG_HEAD_BYTES + i * SG_RIFF_BYTES;
        if (r[2] < 1 || r[2] > SG_RIFF_NOTES) return 0;
        for (j = 0; j < SG_RIFF_NOTES; j++) if (r[3 + j] > 15) return 0;
    }
    return 1;
}

int project_load(void)
{
    static uint8_t data[SG_SAVE_MAX + 1];      /* the extra byte detects an oversized object */
    unsigned i, j, n;
    int size;
    for (i = 0; i < P_NPARAMS; i++) proj.par[i] = om_param_info((int)i)->def;
    sg_init(plat_ms());
    sg_have_save = 0;
    size = plat_store_load(OBJ_PROJ, data, sizeof data);
    if (!sg_valid(data, size)) return 0;      /* another firmware's save, or none: a fresh start */
    proj.par[P_LEVEL] = data[6];
    sg.character = data[8]; sg.onboarded = data[9]; sg.thoughts_on = data[10]; sg.sleeping = data[11];
    sg.energy = data[12]; sg.mood = data[13]; sg.rhythm = data[14]; sg.stage = data[15];
    sg.total_days = get16(data + 16); sg.streak = get16(data + 18); sg.best_streak = get16(data + 20);
    sg.waves = get16(data + 22); sg.lessons = get16(data + 24); sg.slips = get16(data + 26);
    sg.sober_sessions = get16(data + 28);
    sg.thought_next = data[30];
    sg.last_thought_day = get16(data + 32); sg.last_pro_day = get16(data + 34);
    sg.nriffs = data[36];
    for (i = 0, n = SG_HEAD_BYTES; i < sg.nriffs; i++, n += SG_RIFF_BYTES) {
        sg.riffs[i].day = get16(data + n); sg.riffs[i].n = data[n + 2];
        for (j = 0; j < SG_RIFF_NOTES; j++) sg.riffs[i].notes[j] = data[n + 3 + j];
    }
    {
        int s = sg_stage_for(sg.total_days, sg.lessons);   /* the stage follows the counters */
        if (s > sg.stage) sg.stage = (uint8_t)s;
    }
    for (i = 0; i < (unsigned)size; i++) sg_saved[i] = data[i];
    sg_saved_len = (unsigned)size;
    sg_have_save = 1;
    return 1;
}

void project_apply(void)
{
    int i;
    for (i = 0; i < P_NPARAMS; i++) om_set(i, proj.par[i]);
}

int sg_needs_save(void)
{
    static uint8_t data[SG_SAVE_MAX];
    unsigned i, n;
    if (!sg_have_save) return 1;
    n = sg_pack(data);
    if (n != sg_saved_len) return 1;
    for (i = 0; i < n; i++) if (data[i] != sg_saved[i]) return 1;
    return 0;
}

int sg_save(void)
{
    static uint8_t data[SG_SAVE_MAX];
    unsigned i, n;
    int rc;
    if (!sg_needs_save()) return 0;
    n = sg_pack(data);
    rc = plat_store_save(OBJ_PROJ, data, n);
    if (rc) return rc;
    for (i = 0; i < n; i++) sg_saved[i] = data[i];
    sg_saved_len = n;
    sg_have_save = 1;
    return 0;
}
