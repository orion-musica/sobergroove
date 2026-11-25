/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "../dsp/omni.h"
#include "sg_state.h"
#ifndef OM_VERSION
#define OM_VERSION "0.1.0-ALPHA"
#endif
typedef struct { int16_t par[P_NPARAMS]; } project_t;
extern project_t proj;
int project_load(void);
void project_apply(void);
int sg_needs_save(void);
int sg_save(void);              /* 0 success; a failure leaves the state dirty for a later retry */
void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
void ui_say(const char *a, const char *b);
