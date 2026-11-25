/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove: the buddy's state. Two axes of growth: total sober days (never fall) and
 * lessons taught by the user. The stage needs both. Nothing here can die or be lost. */
#pragma once
#include <stdint.h>

enum { CH_LOW, CH_KIT, CH_VINYL, CH_N };
#define SG_STAGES 5
#define SG_MAX_RIFFS 48
#define SG_RIFF_NOTES 4
#define SG_METER_FLOOR 10

typedef struct {
    uint16_t day;                   /* total sober days when it was learned */
    uint8_t n;                      /* notes used, 1..SG_RIFF_NOTES */
    uint8_t notes[SG_RIFF_NOTES];   /* pentatonic degree 0..15 (Kit: drum = degree % 4) */
} sg_riff_t;

typedef struct {
    /* saved */
    uint8_t character, onboarded, thoughts_on, sleeping;
    uint8_t energy, mood, rhythm;   /* SG_METER_FLOOR .. 100 */
    uint8_t stage;                  /* 1 .. SG_STAGES, recomputed from days + lessons */
    uint16_t total_days, streak, best_streak, waves, lessons, slips, sober_sessions;
    uint8_t thought_next;
    uint16_t last_thought_day, last_pro_day;
    uint8_t nriffs;
    sg_riff_t riffs[SG_MAX_RIFFS];
    /* session only */
    uint8_t declared_this_boot;
    uint32_t declared_at, decay_at, sleep_at;
} sg_t;
extern sg_t sg;

void sg_init(uint32_t now);
void sg_tick(uint32_t now);                     /* meters drift slowly, never below the floor */
int sg_can_declare(uint32_t now);               /* 0 already counted today, 1 yes, 2 ask "a new day?" */
void sg_declare(uint32_t now);                  /* today counts */
void sg_slip(void);                             /* the streak restarts; days, waves, stage stay */
void sg_wave_ridden(void);
int sg_add_riff(const uint8_t *notes, int n);   /* lessons +1; returns the riff number (1-based) */
int sg_stage_for(uint16_t days, uint16_t lessons);
int sg_update_stage(void);                      /* returns the new stage if it rose, else 0 */
void sg_care(int action);                       /* 0 meal, 1 talk, 2 move, 3 sleep/wake */
void sg_mood_report(int rating);                /* 1..5 from the talk screen */
const char *sg_stage_name(int stage);
const char *sg_char_name(int ch);
uint16_t sg_stage_days(int stage);
uint16_t sg_stage_lessons(int stage);
