/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove sound: a small synthesizer for the buddy's instruments.
 * Single UI producer (the main loop), audio-IRQ consumer (om_render). Only om_render
 * touches live DSP state; the UI posts commands into a lock-free ring. */
#pragma once
#include <stdint.h>
#define FMO_KEYS 27
#define FMO_VOICES 8

/* Saved sound settings (project.c). */
enum { P_LEVEL, P_NPARAMS };
typedef struct { const char *name; int16_t lo, hi, def; } om_param_t;
const om_param_t *om_param_info(int p);
void om_set(int p, int value);

/* Instruments. The keyboard plays the buddy's instrument; om_note plays any of them. */
enum { INS_BASS, INS_DRUMS, INS_PLUCK, INS_BELL, INS_NINS };
/* Drum pitches for INS_DRUMS (om_note pitch, or white key index % 4 on the keyboard). */
enum { DR_KICK, DR_SNARE, DR_HAT, DR_CLAP };

void om_init(void);
void om_instrument(int ins);                 /* the keyboard's instrument */
void om_trigger(int key, int velocity);      /* white keys only: pentatonic, A minor */
void om_note_off(int key);
/* A timed note, released after dur_ms. pitch: pentatonic degree 0..15 (C3 .. C6), or DR_*. */
void om_note(int ins, int pitch, int velocity, int dur_ms);
void om_panic(void);                         /* silence everything at the next render */
void om_render(int32_t *out, uint32_t n, uint32_t gain_q12);
int om_white_index(int key);                 /* 0..15, or -1 for a black key */
const char *om_sample_name(int key);         /* note name of a white key, "" otherwise */

extern volatile uint32_t om_active_keys, om_triggers, om_notes, om_dropped;
