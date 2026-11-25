/* SPDX-License-Identifier: GPL-3.0-only */
#include "sg_state.h"
sg_t sg;

static const uint16_t STAGE_DAYS[SG_STAGES] = {0, 3, 14, 30, 90};
static const uint16_t STAGE_LESSONS[SG_STAGES] = {0, 3, 10, 20, 40};
static const char *const STAGE_NAMES[SG_STAGES] = {"LOST", "STREET", "GARAGE", "CLUB", "MAIN STAGE"};
static const char *const CHAR_NAMES[CH_N] = {"LOW", "KIT", "VINYL"};
#define DAY_MS 72000000u            /* 20 h: no real-time clock, so a new day is 20 h of uptime */
#define DECAY_MS 180000u            /* one point every 3 minutes awake */
#define SLEEP_GAIN_MS 15000u

uint16_t sg_stage_days(int s) { return STAGE_DAYS[(s < 1 ? 1 : s > SG_STAGES ? SG_STAGES : s) - 1]; }
uint16_t sg_stage_lessons(int s) { return STAGE_LESSONS[(s < 1 ? 1 : s > SG_STAGES ? SG_STAGES : s) - 1]; }
const char *sg_stage_name(int s) { return STAGE_NAMES[(s < 1 ? 1 : s > SG_STAGES ? SG_STAGES : s) - 1]; }
const char *sg_char_name(int c) { return CHAR_NAMES[c >= 0 && c < CH_N ? c : 0]; }

static uint8_t clamp_meter(int v) { return (uint8_t)(v < SG_METER_FLOOR ? SG_METER_FLOOR : v > 100 ? 100 : v); }

void sg_init(uint32_t now)
{
    unsigned i;
    sg.character = CH_LOW; sg.onboarded = 0; sg.thoughts_on = 1; sg.sleeping = 0;
    sg.energy = 70; sg.mood = 60; sg.rhythm = 60; sg.stage = 1;
    sg.total_days = sg.streak = sg.best_streak = sg.waves = sg.lessons = sg.slips = sg.sober_sessions = 0;
    sg.thought_next = 0; sg.last_thought_day = 0xFFFFu; sg.last_pro_day = 0xFFFFu;
    sg.nriffs = 0;
    for (i = 0; i < SG_MAX_RIFFS; i++) sg.riffs[i] = (sg_riff_t){0};
    sg.declared_this_boot = 0; sg.declared_at = now; sg.decay_at = now; sg.sleep_at = now;
}

void sg_tick(uint32_t now)
{
    if (sg.sleeping) {
        while (now - sg.sleep_at >= SLEEP_GAIN_MS) {
            sg.sleep_at += SLEEP_GAIN_MS;
            sg.energy = clamp_meter(sg.energy + 1);
        }
        sg.decay_at = now;
        return;
    }
    sg.sleep_at = now;
    while (now - sg.decay_at >= DECAY_MS) {
        sg.decay_at += DECAY_MS;
        sg.energy = clamp_meter(sg.energy - 1);
        sg.rhythm = clamp_meter(sg.rhythm - 1);
        if ((sg.decay_at / DECAY_MS) & 1u) sg.mood = clamp_meter(sg.mood - 1);
    }
}

int sg_can_declare(uint32_t now)
{
    if (sg.declared_this_boot) return now - sg.declared_at >= DAY_MS ? 1 : 0;
    return sg.total_days ? 2 : 1;
}

void sg_declare(uint32_t now)
{
    sg.declared_this_boot = 1; sg.declared_at = now;
    if (sg.total_days < 60000u) sg.total_days++;
    if (sg.streak < 60000u) sg.streak++;
    if (sg.streak > sg.best_streak) sg.best_streak = sg.streak;
    sg.mood = clamp_meter(sg.mood + 10);
    sg.sleeping = 0;
}

void sg_slip(void)
{
    sg.streak = 0;
    if (sg.slips < 60000u) sg.slips++;
}

void sg_wave_ridden(void)
{
    if (sg.waves < 60000u) sg.waves++;
    sg.mood = clamp_meter(sg.mood + 8);
}

int sg_add_riff(const uint8_t *notes, int n)
{
    unsigned i, slot;
    if (n < 1) return 0;
    if (n > SG_RIFF_NOTES) n = SG_RIFF_NOTES;
    if (sg.nriffs < SG_MAX_RIFFS) slot = sg.nriffs++;
    else {                                       /* full: the oldest riff makes room */
        for (i = 1; i < SG_MAX_RIFFS; i++) sg.riffs[i - 1] = sg.riffs[i];
        slot = SG_MAX_RIFFS - 1u;
    }
    sg.riffs[slot].day = sg.total_days;
    sg.riffs[slot].n = (uint8_t)n;
    for (i = 0; i < SG_RIFF_NOTES; i++) sg.riffs[slot].notes[i] = i < (unsigned)n ? notes[i] : 0;
    if (sg.lessons < 60000u) sg.lessons++;
    sg.rhythm = clamp_meter(sg.rhythm + 6);
    sg.mood = clamp_meter(sg.mood + 6);
    return (int)sg.lessons;
}

int sg_stage_for(uint16_t days, uint16_t lessons)
{
    int s = 1;
    while (s < SG_STAGES && days >= STAGE_DAYS[s] && lessons >= STAGE_LESSONS[s]) s++;
    return s;
}

int sg_update_stage(void)
{
    int s = sg_stage_for(sg.total_days, sg.lessons);
    if (s > sg.stage) { sg.stage = (uint8_t)s; return s; }
    return 0;                                    /* a stage never falls */
}

void sg_care(int action)
{
    switch (action) {
    case 0: sg.rhythm = clamp_meter(sg.rhythm + 20); sg.mood = clamp_meter(sg.mood + 4); break;
    case 1: sg.mood = clamp_meter(sg.mood + 8); break;
    case 2: sg.energy = clamp_meter(sg.energy + 12); sg.rhythm = clamp_meter(sg.rhythm + 6); break;
    default: sg.sleeping = !sg.sleeping; break;
    }
}

void sg_mood_report(int rating)
{
    sg.mood = clamp_meter(sg.mood + (rating >= 4 ? 8 : 4));     /* talking always helps a little */
}
