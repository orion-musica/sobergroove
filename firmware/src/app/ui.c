/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove UI: a recovery companion for musicians on the M-VAVE FM-1.
 * Input runs every loop; the screen is redrawn only when something changed or an
 * animation frame is due, as one 240 x 240 canvas. Nothing here runs in the audio interrupt. */
#include "sg_art.h"
#include "sg_text.h"

static const uint8_t WHITE_K[16] = {0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const uint8_t BLACK_K[11] = {1,3,5,8,10,13,15,17,20,22,25};
#define KEY_SOS 4                       /* the fifth black key */
#define CARE_KEYS ((1u << 1) | (1u << 3) | (1u << 5) | (1u << 8))
#define SOS_KEY_BIT (1u << 10)
#define WHITE_KEYS 0x5AD5AD5u           /* bits of WHITE_K */
#define HOME_HOLD_MS 1500u
#define SOS_HOLD_MS 2000u
#define SOS_MS 1200000u                 /* a craving wave: about 20 minutes */
#define HALT_AT_MS 300000u
#define HALFWAY_MS 600000u
#define BREATH_MS 10000u                /* 5 s in with the bass, 5 s out in silence: 6 breaths a minute */
#define ANIM_MS 450u

enum {
    S_SPLASH, S_WHAT, S_SAFETY, S_CHOOSE, S_P_KNOCK, S_P_WHO, S_P_WHY, S_P_CRASH, S_P_THANKS, S_P_ROAD,
    S_HELP_INTRO, S_READY,
    S_MAIN, S_STATUS, S_STATUS2, S_NEWDAY, S_DECLARED, S_DONE_TODAY, S_THOUGHT, S_ABOUT_PRO, S_ABOUT_GROUPS,
    S_MOOD, S_SOS, S_HALT, S_TIP, S_SOS_EXIT, S_SOS_DONE,
    S_MENU, S_JAM_REC, S_JAM_LISTEN, S_JAM_RESULT, S_REP, S_HELP, S_GROUPS,
    S_SLIP_CONFIRM, S_SLIP_THANKS, S_SLIP_WHAT, S_SLIP_WHO, S_SLIP_DONE, S_SETTINGS, S_LEVELUP
};
enum { CARE_NONE, CARE_MEAL, CARE_MOVE, CARE_SLEEP, CARE_WAKE, CARE_TALK };

typedef struct { uint32_t at; uint8_t ins, pitch, vel; uint16_t dur; } seq_ev_t;
#define NSEQ 24

static struct {
    uint8_t scr, after, sel, redraw, frame, inv;
    uint32_t keys, buttons, entered, anim_at;
    uint32_t home_down, sos_down;
    uint8_t home_fired, sos_fired;
    const char *msg; char msg_sub[32]; uint32_t msg_until;
    uint8_t care; uint32_t care_until;
    /* SOS: the wave keeps time even while a tip or a lesson is on screen */
    uint8_t sos_on, halt_asked, halt_pick; uint32_t sos_start, breath_at, tip_until; uint8_t breath_in;
    /* lessons */
    uint8_t jam_n, jam_notes[SG_RIFF_NOTES], jam_try[SG_RIFF_NOTES], jam_played, jam_ok, jam_tries, jam_from_sos;
    uint32_t jam_at; int riff_no;
    seq_ev_t seq[NSEQ]; uint8_t nseq;
    uint8_t mood_pick, levelup, thought, rep_top;
    uint32_t last_change, last_save, last_check; uint8_t important, dirty_state;
    uint32_t rng;
} ui;

/* ---------------------------------------------------------------- helpers --- */
static void decimal(int v, char *b) { char t[12]; int n = 0; if (v < 0) v = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n) *b++ = t[--n]; *b = 0; }
static char *append(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char *append_num(char *d, int v) { decimal(v, d); while (*d) d++; return d; }
static char *append_2(char *d, int v) { *d++ = (char)('0' + (v / 10) % 10); *d++ = (char)('0' + v % 10); *d = 0; return d; }
static uint32_t lcg(void) { ui.rng = ui.rng * 1664525u + 1013904223u; return ui.rng >> 8; }
static int white_index_of(int k) { int i; for (i = 0; i < 16; i++) if (WHITE_K[i] == k) return i; return -1; }
static int black_index_of(int k) { int i; for (i = 0; i < 11; i++) if (BLACK_K[i] == k) return i; return -1; }
static int buddy_instrument(void) { return sg.character == CH_KIT ? INS_DRUMS : sg.character == CH_VINYL ? INS_PLUCK : INS_BASS; }
static const char *note_label(int pitch)
{
    static const char *const DRUMS[4] = {"KICK", "SNARE", "HAT", "CLAP"};
    static const char *const NOTES[16] = {"C3","D3","E3","G3","A3","C4","D4","E4","G4","A4","C5","D5","E5","G5","A5","C6"};
    if (sg.character == CH_KIT) return DRUMS[pitch & 3];
    return NOTES[pitch & 15];
}
static void go(int s) { ui.scr = (uint8_t)s; ui.sel = 0; ui.entered = plat_ms(); ui.redraw = 1; }
static void changed(int important) { ui.last_change = plat_ms(); ui.dirty_state = 1; if (important) ui.important = 1; }
static void toast(const char *a, const char *b, uint32_t ms)
{
    int i;
    ui.msg = a;
    for (i = 0; i < 31 && b && b[i]; i++) ui.msg_sub[i] = b[i];
    ui.msg_sub[i] = 0;
    ui.msg_until = plat_ms() + ms; ui.redraw = 1;
}
int ui_dirty(void) { return ui.dirty_state; }
void ui_say(const char *a, const char *b) { toast(a, b, 1800u); }

/* a tiny score player for chimes, fanfares, breaths and riffs */
static void seq_add(uint32_t delay, int ins, int pitch, int vel, int dur)
{
    if (ui.nseq >= NSEQ) return;
    ui.seq[ui.nseq++] = (seq_ev_t){plat_ms() + delay, (uint8_t)ins, (uint8_t)pitch, (uint8_t)vel, (uint16_t)dur};
}
static void seq_tick(uint32_t now)
{
    unsigned i = 0;
    while (i < ui.nseq) {
        if ((int32_t)(now - ui.seq[i].at) >= 0) {
            om_note(ui.seq[i].ins, ui.seq[i].pitch, ui.seq[i].vel, ui.seq[i].dur);
            ui.seq[i] = ui.seq[--ui.nseq];
        } else i++;
    }
}
static void seq_clear(void) { ui.nseq = 0; }
static void chime(void) { seq_add(0, INS_BELL, 9, 70, 300); seq_add(110, INS_BELL, 12, 60, 500); }
static void fanfare(void)
{
    static const uint8_t P[5] = {5, 7, 9, 10, 12};
    int i;
    for (i = 0; i < 5; i++) seq_add((uint32_t)i * 130u, INS_BELL, P[i], 85, i == 4 ? 1100 : 260);
    seq_add(520, INS_BASS, 5, 80, 900);
}
static void play_riff(const uint8_t *notes, int n, uint32_t delay)
{
    int i, ins = buddy_instrument();
    for (i = 0; i < n; i++) seq_add(delay + (uint32_t)i * 380u, ins, notes[i], 100, 320);
}

/* ------------------------------------------------------------------ drawing --- */
#define TOP 24
#define BOTTOM 216
static uint16_t ink, paper;
static void text_c(int y, const felucca_font_t *f, const char *s, uint16_t c)
{
    char fit[48]; int n = 0;
    if (!s) s = "";
    while (s[n] && n < 47) { fit[n] = s[n]; n++; }
    fit[n] = 0;
    while (n && text_w(f, fit) > 228) fit[--n] = 0;
    cv_text((240 - text_w(f, fit)) / 2, y, f, fit, c);
}
static void header(const char *l, const char *r)
{
    cv_text(6, 4, &FONT_B, l ? l : "", ink);
    if (r && *r) cv_text(234 - text_w(&FONT_B, r), 4, &FONT_B, r, ink);
    cv_rect(4, 21, 232, 1, ink);
}
static void footer(const char *hint)
{
    cv_rect(4, 218, 232, 1, ink);
    text_c(224, &FONT_XS, hint, ink);
}
static int count_lines(const char *const *l) { int n = 0; if (l) while (l[n]) n++; return n; }

/* the page model: art on top, lines, options; anything else is drawn by the caller */
typedef struct {
    const char *tl, *tr, *hint;
    const char *const *lines;
    const char *const *opts;
    int art_scale;                      /* 0 = no art; the grid is in px_rows */
    int extra_h;                        /* reserved height for the caller's drawing */
    int big;                            /* lines in the big face */
} page_t;
static int extra_y;

static void draw_page(const page_t *p)
{
    int nl = count_lines(p->lines), no = count_lines(p->opts), h = 0, y, i;
    int art_h = p->art_scale ? GRID_H * p->art_scale : 0;
    int lh = p->big ? 26 : 18;
    h += art_h + (art_h && (nl || no || p->extra_h) ? 6 : 0);
    h += nl * lh;
    h += p->extra_h + (p->extra_h && no ? 6 : 0);
    h += no ? 6 + no * 20 : 0;
    y = TOP + (BOTTOM - TOP - h) / 2;
    if (y < TOP + 2) y = TOP + 2;
    header(p->tl, p->tr);
    if (art_h) {
        px_draw((240 - GRID_W * p->art_scale) / 2, y, p->art_scale, ink);
        y += art_h + 6;
    }
    for (i = 0; i < nl; i++, y += lh)
        text_c(y + 1, p->big ? &FONT_M : &FONT_B, p->lines[i], ink);
    extra_y = y;
    y += p->extra_h + (p->extra_h && no ? 6 : 0);
    if (no) y += 6;
    for (i = 0; i < no; i++, y += 20) {
        if (i == ui.sel) {
            cv_rect(20, y - 1, 200, 19, ink);
            text_c(y + 1, &FONT_B, p->opts[i], paper);
        } else text_c(y + 1, &FONT_B, p->opts[i], ink);
    }
    footer(p->hint);
}
static void draw_meter(int x, int y, const char *label, int v)
{
    int k, filled = (v + 10) / 20;
    cv_text(x, y, &FONT_XS, label, ink);
    for (k = 0; k < 5; k++) {
        cv_rect(x + 22 + k * 9, y + 2, 8, 8, ink);
        if (k >= filled) cv_rect(x + 23 + k * 9, y + 3, 6, 6, paper);
    }
}
static void draw_bar(int y, int num, int den, const char *label)
{
    int w = den ? 200 * (num > den ? den : num) / den : 200;
    cv_rect(20, y, 200, 12, ink);
    cv_rect(21, y + 1, 198, 10, paper);
    cv_rect(21, y + 1, w > 198 ? 198 : w, 10, ink);
    text_c(y + 15, &FONT_XS, label, ink);
}
static void draw_scale5(int y, int pick)
{
    int k;
    for (k = 0; k < 5; k++) {
        int x = 47 + k * 30;
        char d[2] = {(char)('1' + k), 0};
        cv_rect(x, y, 26, 26, ink);
        if (k + 1 != pick) cv_rect(x + 1, y + 1, 24, 24, paper);
        cv_text(x + 9, y + 5, &FONT_B, d, k + 1 == pick ? paper : ink);
    }
}
static void draw_slots(int y, const uint8_t *notes, int n, const uint8_t *want, int shown)
{
    int k;
    for (k = 0; k < SG_RIFF_NOTES; k++) {
        int x = 16 + k * 53;
        cv_rect(x, y, 49, 22, ink);
        cv_rect(x + 1, y + 1, 47, 20, paper);
        if (k < n && k < shown) {
            const char *lab = note_label(notes[k]);
            if (want && want[k] != notes[k]) {            /* a wrong note: dashed box */
                int i;
                for (i = 0; i < 47; i += 4) { cv_rect(x + 1 + i, y + 1, 2, 2, ink); cv_rect(x + 1 + i, y + 19, 2, 2, ink); }
            }
            cv_text(x + (49 - text_w(&FONT_B, lab)) / 2, y + 3, &FONT_B, lab, ink);
        }
    }
}
static void draw_toast(void)
{
    int h = ui.msg_sub[0] ? 40 : 24;
    cv_rect(8, 192 - h, 224, h, ink);
    text_c(192 - h + 4, &FONT_B, ui.msg, paper);
    if (ui.msg_sub[0]) text_c(192 - h + 22, &FONT_B, ui.msg_sub, paper);
}

/* builds the main scene into px_rows */
static void main_scene(void)
{
    int closed = sg.sleeping, dy = (!sg.sleeping && ui.frame) ? 1 : 0;
    px_clear();
    px_stage_scene(sg.stage, sg.character, closed, dy);
    if (sg.sleeping) { PX(BM_MOON, 0, 0); if (ui.frame) PX(BM_Z, 27, 3); else PX(BM_ZS, 27, 8); }
    else if (ui.frame && sg.stage < 5) PX(BM_NOTE, sg.stage == 3 ? 26 : 0, 3);
    if (ui.care && (int32_t)(plat_ms() - ui.care_until) < 0) {
        if (ui.care == CARE_MEAL) { PX(BM_BOWL, 0, 19); PX(BM_STEAM, 0, 15); PX(BM_HEART, 26, 1); }
        else if (ui.care == CARE_MOVE) PX(BM_DUST, 6, 24);
        else if (ui.care == CARE_TALK) PX(BM_HEART, 26, 1);
        else if (ui.care == CARE_WAKE) PX(BM_SUN, 25, 1);
    }
}
static void sos_scene(void)
{
    uint32_t t = plat_ms() - ui.sos_start;
    int amp = t < SOS_MS / 4 ? 1 : t < SOS_MS * 3 / 4 ? (t > SOS_MS * 3 / 8 && t < SOS_MS * 5 / 8 ? 3 : 2) : 1;
    px_clear();
    px_sprite(sg.character, 6, ui.breath_in ? 6 : 7, 0);
    px_wave(amp, (int)(t / 450u));
}
static void small_buddy(int closed)
{
    px_clear();
    px_sprite(sg.character, 6, 4, closed);
}
static void draw_trio(int y, int pick)
{
    int c;
    for (c = 0; c < CH_N; c++) {
        px_clear();
        px_sprite(c, 0, 0, 0);
        px_draw(18 + c * 72, y, 3, (pick < 0 || pick == c) ? ink : LCD_DIM);
    }
}

static void render(void)
{
    static char a[40], b[40], c[40], d[40];
    static const char *L[8];
    static const char *O[8];
    page_t p = {0};
    uint32_t now = plat_ms();
    int s = ui.scr;
    ui.inv = (s == S_LEVELUP);
    ink = ui.inv ? LCD_BG : LCD_INK;
    paper = ui.inv ? LCD_INK : LCD_BG;
    cv_begin(240, 240, paper);
    switch (s) {
    case S_SPLASH:
        L[0] = "STAY OFF ALCOHOL."; L[1] = "ONE DAY AT A TIME."; L[2] = 0;
        p = (page_t){"SOBER GROOVE", "v" OM_VERSION, "FOR MUSICIANS IN RECOVERY", L, 0, 0, 66, 0};
        draw_page(&p); draw_trio(extra_y + 4, -1);
        break;
    case S_WHAT: p = (page_t){"WHAT IS THIS?", "1/4", "SEL NEXT", TXT_WHAT, 0, 0, 0, 0}; draw_page(&p); break;
    case S_SAFETY: p = (page_t){"IMPORTANT", "2/4", "SEL I UNDERSTAND", TXT_SAFETY, 0, 0, 0, 0}; draw_page(&p); break;
    case S_CHOOSE:
        O[0] = "LOW - BASS"; O[1] = "KIT - DRUMS"; O[2] = "VINYL - DJ"; O[3] = 0;
        p = (page_t){"CHOOSE YOUR BUDDY", "3/4", "KNOB PICK - SEL OK", 0, O, 0, 66, 0};
        draw_page(&p); draw_trio(extra_y, ui.sel);
        break;
    case S_P_KNOCK:
        L[0] = "KNOCK KNOCK."; L[1] = 0;
        px_clear(); px_door(sg.character, ui.frame);
        p = (page_t){"PROLOGUE", "1/6", "SEL OPEN THE DOOR", L, 0, 5, 0, 0}; draw_page(&p); break;
    case S_P_WHO:
        px_clear(); px_sprite(sg.character, 6, 5, 0); PX(BM_CLOUD, 23, 0);
        p = (page_t){"PROLOGUE", "2/6", "SEL NEXT", TXT_WHO[sg.character], 0, 4, 0, 0}; draw_page(&p); break;
    case S_P_WHY:
        px_clear(); px_sprite(sg.character, 6, 6, 0);
        p = (page_t){"PROLOGUE", "3/6", "SEL NEXT", TXT_WHY, 0, 5, 0, 0}; draw_page(&p); break;
    case S_P_CRASH:
        O[0] = "COME IN"; O[1] = 0;
        px_clear(); px_door(sg.character, 0);
        p = (page_t){"PROLOGUE", "4/6", "SEL", TXT_CRASH, O, 4, 0, 0}; draw_page(&p); break;
    case S_P_THANKS:
        px_clear(); px_slumped(sg.character, 0);
        p = (page_t){"PROLOGUE", "5/6", "SEL NEXT", TXT_THANKS, 0, 5, 0, 0}; draw_page(&p); break;
    case S_P_ROAD:
        px_clear(); px_slumped(sg.character, 0); PX(BM_HEART, 26, 2);
        p = (page_t){"PROLOGUE", "6/6", "SEL NEXT", TXT_SAMEROAD, 0, 4, 0, 0}; draw_page(&p); break;
    case S_HELP_INTRO: p = (page_t){"HELP IS HERE", "4/4", "SEL NEXT", TXT_HELP, 0, 0, 0, 0}; draw_page(&p); break;
    case S_READY:
        px_clear(); px_slumped(sg.character, 0); PX(BM_SUN, 25, 0);
        p = (page_t){"READY", "DAY 0", "SEL START", TXT_READY, 0, 4, 0, 0}; draw_page(&p); break;
    case S_MAIN: {
        char *q = append(a, "DAY "); append_num(q, sg.total_days);
        header(a, sg_stage_name(sg.stage));
        main_scene();
        px_draw(24, 26, 6, ink);
        draw_meter(8, 193, "EN", sg.energy);
        draw_meter(84, 193, "MO", sg.mood);
        draw_meter(160, 193, "RH", sg.rhythm);
        footer("HOME STATUS - KEY 5 SOS - GLO MENU");
        break;
    }
    case S_STATUS: {
        int y = 96;
        char *q = append(b, "STREAK "); q = append_num(q, sg.streak); q = append(q, " - BEST "); append_num(q, sg.best_streak);
        q = append(c, "WAVES RIDDEN "); append_num(q, sg.waves);
        q = append(d, "LESSONS "); q = append_num(q, sg.lessons); q = append(q, " - RIFFS "); append_num(q, sg.nriffs);
        decimal(sg.total_days, a);
        header("STATUS", sg_stage_name(sg.stage));
        /* the total, the number that never falls, on top and largest */
        text_c(32, &FONT_L, a, ink);
        text_c(68, &FONT_XS, "SOBER DAYS", ink);
        text_c(y, &FONT_B, b, ink); y += 22;
        text_c(y, &FONT_B, c, ink); y += 22;
        text_c(y, &FONT_B, d, ink); y += 30;
        text_c(y, &FONT_XS, sg.sleeping ? "ASLEEP. KEY 4 WAKES ME." : "HOLD HOME EACH MORNING.", ink);
        footer("HOME NEXT - HOLD HOME: NOT TODAY");
        break;
    }
    case S_STATUS2: {
        int next = sg.stage < SG_STAGES ? sg.stage + 1 : SG_STAGES;
        if (sg.stage >= SG_STAGES) {
            L[0] = "MAIN STAGE."; L[1] = "THE CROWD KNOWS"; L[2] = "YOUR RIFFS."; L[3] = 0;
            p = (page_t){"NEXT STAGE", "-", "HOME BACK", L, 0, 0, 0, 0}; draw_page(&p);
        } else {
            char *q = append(a, "NEXT: "); append(q, sg_stage_name(next));
            q = append(b, "SOBER DAYS "); q = append_num(q, sg.total_days); q = append(q, "/"); append_num(q, sg_stage_days(next));
            q = append(c, "LESSONS "); q = append_num(q, sg.lessons); q = append(q, "/"); append_num(q, sg_stage_lessons(next));
            L[0] = a; L[1] = 0;
            p = (page_t){"NEXT STAGE", sg_stage_name(next), "HOME BACK", L, 0, 0, 84, 0};
            draw_page(&p);
            draw_bar(extra_y + 8, sg.total_days, sg_stage_days(next), b);
            draw_bar(extra_y + 48, sg.lessons, sg_stage_lessons(next), c);
        }
        break;
    }
    case S_NEWDAY:
        L[0] = "A NEW DAY SINCE"; L[1] = "WE LAST TALKED?"; L[2] = 0;
        O[0] = "YES, A NEW DAY"; O[1] = "NO, SAME DAY"; O[2] = 0;
        p = (page_t){"GOOD MORNING", "", "KNOB PICK - SEL OK", L, O, 0, 0, 0}; draw_page(&p); break;
    case S_DECLARED: {
        char *q = append(a, "DAY "); q = append_num(q, sg.total_days); append(q, ".");
        L[0] = "NOT TODAY."; L[1] = a; L[2] = 0;
        px_clear(); px_sprite(sg.character, 6, 4, 0); PX(BM_SUN, 25, 0); PX(BM_SPARK, 0, 1);
        p = (page_t){"NOT TODAY", sg_stage_name(sg.stage), "SEL OK", L, 0, 4, 0, 0}; draw_page(&p); break;
    }
    case S_DONE_TODAY:
        L[0] = "TODAY ALREADY COUNTS."; L[1] = "KEEP GOING."; L[2] = 0;
        px_clear(); px_sprite(sg.character, 6, 4, 0); PX(BM_HEART, 26, 1);
        p = (page_t){"NOT TODAY", "", "SEL OK", L, 0, 4, 0, 0}; draw_page(&p); break;
    case S_THOUGHT: {
        const thought_t *t = &THOUGHTS[ui.thought % NTHOUGHTS];
        O[0] = "HMM"; O[1] = t->kind == TH_REFLECT ? 0 : "MORE"; O[2] = 0;
        small_buddy(0); PX(BM_Q, 27, 1);
        p = (page_t){"BUDDY THINKS", t->kind == TH_PRO ? "PRO" : t->kind == TH_PEOPLE ? "PEOPLE" : "THOUGHT",
                     "KNOB PICK - SEL OK", t->line, O, 2, 0, 0};
        draw_page(&p); break;
    }
    case S_ABOUT_PRO: p = (page_t){"ABOUT THERAPY", "", "SEL OK", TXT_ABOUT_PRO, 0, 0, 0, 0}; draw_page(&p); break;
    case S_ABOUT_GROUPS: p = (page_t){"ABOUT GROUPS", "", "SEL OK - GLO HELP", TXT_ABOUT_GROUPS, 0, 0, 0, 0}; draw_page(&p); break;
    case S_MOOD:
        L[0] = "I'M LISTENING."; L[1] = "HOW ARE YOU?"; L[2] = 0;
        small_buddy(0); PX(BM_HEART, 26, 1);
        p = (page_t){"TALK", "", "KNOB PICK - SEL OK", L, 0, 3, 30, 0}; draw_page(&p);
        draw_scale5(extra_y + 2, ui.mood_pick);
        break;
    case S_SOS: {
        uint32_t t = now - ui.sos_start;
        int left = (int)(t / 1000u);
        char *q = append_2(a, left / 60); q = append(q, ":"); append_2(q, left % 60);
        q = append(b, a); append(q, " / 20:00");
        L[0] = ui.breath_in ? "BREATHE IN" : "BREATHE OUT";
        L[1] = t < 15000u ? "I'M HERE. WE'LL RIDE IT." :
               (t >= HALFWAY_MS && t < HALFWAY_MS + 20000u) ? "HALFWAY. CALL SOMEONE?" :
               (t > SOS_MS * 3 / 8 && t < SOS_MS * 5 / 8) ? "THIS IS THE PEAK." : "IT WILL PASS.";
        L[2] = 0;
        sos_scene();
        p = (page_t){"SOS", a, "PLAY ALONG - HOLD KEY 5: EXIT", L, 0, 4, 30, 0};
        draw_page(&p);
        draw_bar(extra_y + 2, (int)(t / 1000u), (int)(SOS_MS / 1000u), b);
        break;
    }
    case S_HALT:
        L[0] = "QUICK CHECK."; L[1] = "ARE YOU..."; L[2] = 0;
        O[0] = "1 HUNGRY"; O[1] = "2 ANGRY"; O[2] = "3 LONELY"; O[3] = "4 TIRED"; O[4] = 0;
        p = (page_t){"SOS", "HALT", "BLACK KEYS 1-4", L, O, 0, 0, 0}; draw_page(&p); break;
    case S_TIP:
        if (ui.halt_pick == 1) { O[0] = "TEACH A RIFF"; O[1] = "JUST BREATHE"; O[2] = 0; }
        else O[0] = 0;
        p = (page_t){HALT_ASK[ui.halt_pick & 3], "SOS", "SEL BACK TO THE WAVE", HALT_TIP[ui.halt_pick & 3],
                     ui.halt_pick == 1 ? O : 0, 0, 0, 0};
        draw_page(&p); break;
    case S_SOS_EXIT:
        L[0] = "FEELING LIGHTER?"; L[1] = 0;
        O[0] = "YES, EXIT"; O[1] = "NOT YET, BACK"; O[2] = 0;
        p = (page_t){"SOS", "", "KNOB PICK - SEL OK", L, O, 0, 0, 0}; draw_page(&p); break;
    case S_SOS_DONE: {
        char *q = append(a, "WAVES "); append_num(q, sg.waves);
        L[0] = "WAVE PASSED."; L[1] = "YOU RODE IT OUT."; L[2] = 0;
        px_clear(); px_sprite(sg.character, 6, 3, 0); PX(BM_SPARK, 0, 0); PX(BM_SPARK, 27, 0);
        p = (page_t){"WAVE PASSED", a, "SEL OK", L, 0, 4, 0, 0}; draw_page(&p); break;
    }
    case S_MENU:
        append(append(a, "JAM: TEACH "), sg_char_name(sg.character));
        O[0] = a; O[1] = "REPERTOIRE"; O[2] = "HELP"; O[3] = "THERE WAS A SLIP"; O[4] = "SETTINGS"; O[5] = "BACK"; O[6] = 0;
        p = (page_t){"MENU", "", "KNOB PICK - SEL OK - GLO BACK", 0, O, 0, 0, 0}; draw_page(&p); break;
    case S_JAM_REC:
        append(append(a, "TEACH "), sg_char_name(sg.character));
        L[0] = sg.character == CH_KIT ? "PLAY UP TO 4 HITS" : "PLAY UP TO 4 NOTES";
        L[1] = "ON THE WHITE KEYS"; L[2] = 0;
        p = (page_t){a, sg_stage_name(sg.stage), "SEL DONE - GLO BACK", L, 0, 0, 30, 0};
        draw_page(&p); draw_slots(extra_y + 4, ui.jam_notes, ui.jam_n, 0, ui.jam_n);
        break;
    case S_JAM_LISTEN:
        L[0] = "GOT IT. LET ME TRY..."; L[1] = 0;
        small_buddy(0); if (ui.frame) PX(BM_NOTE, 27, 3);
        append(append(a, sg_char_name(sg.character)), " TRIES");
        p = (page_t){a, "", "LISTEN", L, 0, 3, 30, 0};
        draw_page(&p); draw_slots(extra_y + 4, ui.jam_try, ui.jam_n, ui.jam_notes, ui.jam_played);
        break;
    case S_JAM_RESULT:
        if (ui.jam_ok == ui.jam_n) {
            char *q = append(b, "NEW RIFF #"); append_num(q, ui.riff_no);
            L[0] = "I GOT IT!"; L[1] = b; L[2] = 0; O[0] = "NICE"; O[1] = 0;
            px_clear(); px_sprite(sg.character, 6, 3, 0); PX(BM_SPARK, 0, 0); PX(BM_SPARK, 27, 0);
            p = (page_t){"LESSON +1", "", "SEL OK", L, O, 3, 0, 0};
        } else {
            char *q = append_num(b, ui.jam_ok); q = append(q, " OF "); q = append_num(q, ui.jam_n); append(q, ". CLOSE!");
            L[0] = b; L[1] = 0; O[0] = "TEACH AGAIN"; O[1] = "KEEP IT LIKE THIS"; O[2] = 0;
            p = (page_t){"LESSON", "", "KNOB PICK - SEL OK", L, O, 0, 30, 0};
        }
        draw_page(&p);
        if (ui.jam_ok != ui.jam_n) draw_slots(extra_y + 4, ui.jam_try, ui.jam_n, ui.jam_notes, ui.jam_n);
        break;
    case S_REP: {
        static char rows[6][32];
        int i;
        if (!sg.nriffs) {
            L[0] = "NO RIFFS YET."; L[1] = ""; L[2] = "GLO > JAM TO TEACH"; L[3] = 0;
            p = (page_t){"REPERTOIRE", "0", "GLO BACK", L, 0, 0, 0, 0}; draw_page(&p); break;
        }
        for (i = 0; i < 6 && ui.rep_top + i < sg.nriffs; i++) {
            const sg_riff_t *r = &sg.riffs[sg.nriffs - 1 - (ui.rep_top + i)];      /* newest first */
            char *q = append(rows[i], "#"); int k;
            q = append_2(q, (int)(sg.nriffs - (ui.rep_top + i))); q = append(q, " D"); q = append_num(q, r->day);
            for (k = 0; k < r->n; k++) { q = append(q, " "); q = append(q, note_label(r->notes[k])); }
            O[i] = rows[i];
        }
        O[i] = 0;
        decimal(sg.nriffs, a);
        { uint8_t keep = ui.sel; ui.sel = (uint8_t)(ui.sel - ui.rep_top);
          p = (page_t){"REPERTOIRE", a, "SEL PLAY - GLO BACK", 0, O, 0, 0, 0}; draw_page(&p); ui.sel = keep; }
        break;
    }
    case S_HELP: p = (page_t){"HELP", "", "SEL GROUPS - GLO BACK", TXT_HELP, 0, 0, 0, 0}; draw_page(&p); break;
    case S_GROUPS: p = (page_t){"SUPPORT GROUPS", "", "SEL MORE - GLO BACK", TXT_GROUPS, 0, 0, 0, 0}; draw_page(&p); break;
    case S_SLIP_CONFIRM:
        L[0] = "THERE WAS A SLIP?"; L[1] = 0; O[0] = "YES"; O[1] = "NO, BACK"; O[2] = 0;
        p = (page_t){"SLIP", "", "KNOB PICK - SEL OK", L, O, 0, 0, 0}; draw_page(&p); break;
    case S_SLIP_THANKS:
        L[0] = "THANK YOU FOR"; L[1] = "TELLING ME."; L[2] = "A SLIP IS NOT THE END."; L[3] = 0;
        px_clear(); px_sprite(sg.character, 6, 5, 0); PX(BM_CLOUD, 12, 0); px_bm(ui.frame ? BM_RAIN : BM_RAIN2, 1, 12, 3);
        p = (page_t){"SLIP", "", "SEL NEXT", L, 0, 3, 0, 0}; draw_page(&p); break;
    case S_SLIP_WHAT:
        L[0] = "WHAT WAS IT?"; L[1] = 0;
        p = (page_t){"SLIP", "1/2", "KNOB PICK - SEL OK", L, SLIP_TRIGGERS, 0, 0, 0}; draw_page(&p); break;
    case S_SLIP_WHO:
        L[0] = "WHO CAN HELP YOU"; L[1] = "TODAY?"; L[2] = 0;
        p = (page_t){"SLIP", "2/2", "KNOB PICK - SEL OK", L, SLIP_WHO, 0, 0, 0}; draw_page(&p); break;
    case S_SLIP_DONE: {
        char *q = append(b, "SOBER DAYS: "); append_num(q, sg.total_days);
        q = append(c, "STAGE KEPT: "); append(q, sg_stage_name(sg.stage));
        L[0] = "YOUR PROGRESS STAYS."; L[1] = b; L[2] = c; L[3] = "STILL ON YOUR COUCH."; L[4] = 0;
        px_clear(); px_sprite(sg.character, 6, 4, 0); PX(BM_SUN, 0, 0);
        p = (page_t){"SLIP", "", "SEL OK", L, 0, 3, 0, 0}; draw_page(&p); break;
    }
    case S_SETTINGS: {
        append(append(a, "BUDDY: "), sg_char_name(sg.character));
        append(b, sg.thoughts_on ? "THOUGHTS: ON" : "THOUGHTS: OFF");
        append_num(append(c, "VOLUME: "), proj.par[P_LEVEL]);
        O[0] = a; O[1] = b; O[2] = c; O[3] = "BACK"; O[4] = 0;
        (void)d;
        p = (page_t){"SETTINGS", "", "SEL CHANGE - GLO BACK", 0, O, 0, 0, 0}; draw_page(&p); break;
    }
    case S_LEVELUP: {
        const char *const *ch = (ui.levelup == 4 && sg.character == CH_KIT) ? TXT_CHAPTER4_KIT : TXT_CHAPTER[ui.levelup];
        char *q = append(a, "STAGE "); q = append_num(q, ui.levelup); append(q, "/5");
        px_clear(); px_stage_scene(ui.levelup, sg.character, 0, 0);
        p = (page_t){a, sg_stage_name(ui.levelup), "SEL OK", ch, 0, 4, 0, 0}; draw_page(&p); break;
    }
    default: break;
    }
    if (ui.msg && (int32_t)(now - ui.msg_until) < 0) draw_toast();
    /* two transfers of 57,600 bytes each: below a 16-bit DMA count, like the strips upstream sent */
    lcd_blit(0, 0, 240, 120, cv_px);
    lcd_blit(0, 120, 240, 120, cv_px + 240u * 120u);
}

/* ------------------------------------------------------------------ actions --- */
static void lights(void)
{
    uint32_t b = (1u << B_GLO) | (1u << B_HOME) | (1u << B_SAVE) | (1u << B_SEL), k = 0;
    if (ui.scr == S_MAIN) k = CARE_KEYS | SOS_KEY_BIT;
    else if (ui.scr == S_HALT) k = CARE_KEYS;
    else if (ui.scr == S_JAM_REC) k = WHITE_KEYS;
    else if (ui.scr >= S_MAIN && ui.scr != S_SOS) k = SOS_KEY_BIT;
    if (ui.scr == S_SOS) k = SOS_KEY_BIT | WHITE_KEYS;
    plat_leds(b, k);
}
static int opt_count(void)
{
    switch (ui.scr) {
    case S_CHOOSE: return 3;
    case S_P_CRASH: return 1;
    case S_NEWDAY: case S_SOS_EXIT: case S_SLIP_CONFIRM: return 2;
    case S_THOUGHT: return THOUGHTS[ui.thought % NTHOUGHTS].kind == TH_REFLECT ? 1 : 2;
    case S_HALT: return 4;
    case S_TIP: return ui.halt_pick == 1 ? 2 : 0;
    case S_MENU: return 6;
    case S_JAM_RESULT: return ui.jam_ok == ui.jam_n ? 1 : 2;
    case S_REP: return sg.nriffs;
    case S_SLIP_WHAT: return 5;
    case S_SLIP_WHO: return 4;
    case S_SETTINGS: return 4;
    default: return 0;
    }
}
static void after_lesson(void);
static int pick_thought(void)
{
    int i;
    if (!sg.thoughts_on || sg.last_thought_day == sg.total_days) return -1;
    for (i = 0; i < NTHOUGHTS; i++) {
        int idx = (sg.thought_next + i) % NTHOUGHTS;
        if (THOUGHTS[idx].kind == TH_PRO && sg.last_pro_day != 0xFFFFu && (uint16_t)(sg.total_days - sg.last_pro_day) < 7u) continue;
        sg.thought_next = (uint8_t)((idx + 1) % NTHOUGHTS);
        sg.last_thought_day = sg.total_days;
        if (THOUGHTS[idx].kind == TH_PRO) sg.last_pro_day = sg.total_days;
        changed(0);
        return idx;
    }
    return -1;
}
/* after a day, a lesson or a slip: a new stage first, then maybe a thought, then home */
static void continue_home(void)
{
    int t;
    if (ui.levelup) { go(S_LEVELUP); fanfare(); return; }
    if (ui.scr == S_DECLARED && (t = pick_thought()) >= 0) { ui.thought = (uint8_t)t; go(S_THOUGHT); return; }
    go(S_MAIN);
}
static void declare(void)
{
    sg_declare(plat_ms());
    {
        int s = sg_update_stage();
        ui.levelup = (uint8_t)(s > 0 ? s : 0);
    }
    changed(1);
    fanfare();
    go(S_DECLARED);
}
static void start_sos(void)
{
    uint32_t now = plat_ms();
    ui.sos_on = 1; ui.sos_start = now; ui.breath_at = now; ui.breath_in = 1; ui.halt_asked = 0;
    seq_clear();
    om_instrument(INS_BELL);
    om_note(INS_BASS, 0, 75, 4600);
    sg.sleeping = 0;
    go(S_SOS);
}
static void end_sos(int ridden)
{
    ui.sos_on = 0;
    om_instrument(buddy_instrument());
    if (ridden) { sg_wave_ridden(); changed(1); fanfare(); go(S_SOS_DONE); }
    else { go(S_MAIN); toast("OK. I'M HERE", "IF IT COMES BACK.", 2600u); }
}
static void jam_start(int from_sos)
{
    ui.jam_n = 0; ui.jam_played = 0; ui.jam_tries = 0; ui.jam_from_sos = (uint8_t)from_sos;
    om_instrument(buddy_instrument());
    go(S_JAM_REC);
}
static void jam_listen(void)
{
    static const uint8_t ERR[SG_STAGES] = {25, 15, 8, 4, 0};
    int i, err = ERR[sg.stage - 1] >> (ui.jam_tries > 2 ? 2 : ui.jam_tries);
    ui.jam_ok = 0;
    for (i = 0; i < ui.jam_n; i++) {
        uint8_t n = ui.jam_notes[i];
        if ((int)(lcg() % 100u) < err) {
            int step = sg.character == CH_KIT ? 1 : ((lcg() & 1u) ? 1 : -1);
            n = (uint8_t)((n + 16 + step) % 16);
        }
        ui.jam_try[i] = n;
        if (n == ui.jam_notes[i]) ui.jam_ok++;
    }
    ui.jam_played = 0; ui.jam_at = plat_ms();
    go(S_JAM_LISTEN);
}
static void jam_keep(void)
{
    int s;
    sg_add_riff(ui.jam_notes, ui.jam_n);
    ui.riff_no = sg.nriffs;
    s = sg_update_stage();
    if (s > 0) ui.levelup = (uint8_t)s;
    changed(1);
}
static void after_lesson(void)
{
    if (ui.levelup) { go(S_LEVELUP); fanfare(); return; }
    if (ui.jam_from_sos && ui.sos_on) { om_instrument(INS_BELL); go(S_SOS); return; }
    go(S_MAIN);
}
static void care(int i)
{
    uint32_t now = plat_ms();
    if (i == 1) { ui.mood_pick = 3; go(S_MOOD); return; }
    sg_care(i);
    changed(0);
    ui.care_until = now + 2600u;
    if (i == 0) { ui.care = CARE_MEAL; toast("THANKS FOR EATING.", "RHYTHM UP.", 2200u); chime(); }
    else if (i == 2) { ui.care = CARE_MOVE; toast("NICE MOVE.", "ENERGY UP.", 2200u); chime(); }
    else if (sg.sleeping) { ui.care = CARE_SLEEP; toast("GOOD NIGHT.", "SEE YOU TOMORROW.", 2200u); seq_add(0, INS_BELL, 5, 50, 900); }
    else { ui.care = CARE_WAKE; toast("GOOD MORNING.", 0, 2000u); chime(); }
    ui.redraw = 1;
}
static void on_sel(void)
{
    int s = ui.scr, n = opt_count();
    switch (s) {
    case S_SPLASH: go(sg.onboarded ? S_MAIN : S_WHAT); break;
    case S_WHAT: go(S_SAFETY); break;
    case S_SAFETY: go(S_CHOOSE); break;
    case S_CHOOSE: sg.character = ui.sel; om_instrument(buddy_instrument()); go(S_P_KNOCK); break;
    case S_P_KNOCK: go(S_P_WHO); break;
    case S_P_WHO: go(S_P_WHY); break;
    case S_P_WHY: go(S_P_CRASH); break;
    case S_P_CRASH: chime(); go(S_P_THANKS); break;
    case S_P_THANKS: go(S_P_ROAD); break;
    case S_P_ROAD: go(S_HELP_INTRO); break;
    case S_HELP_INTRO: go(S_READY); break;
    case S_READY: sg.onboarded = 1; changed(1); go(S_MAIN); break;
    case S_NEWDAY: if (ui.sel == 0) declare(); else { sg.declared_this_boot = 1; sg.declared_at = plat_ms(); go(S_DONE_TODAY); } break;
    case S_DECLARED: continue_home(); break;
    case S_DONE_TODAY: go(S_MAIN); break;
    case S_THOUGHT:
        if (ui.sel == 1) go(THOUGHTS[ui.thought % NTHOUGHTS].kind == TH_PRO ? S_ABOUT_PRO : S_ABOUT_GROUPS);
        else go(S_MAIN);
        break;
    case S_ABOUT_PRO: case S_ABOUT_GROUPS: go(S_MAIN); break;
    case S_MOOD: sg_mood_report(ui.mood_pick); changed(0); ui.care = CARE_TALK; ui.care_until = plat_ms() + 2600u;
        go(S_MAIN); toast("THANKS FOR TELLING ME.", 0, 2200u); chime(); break;
    case S_HALT: ui.halt_pick = ui.sel; ui.tip_until = plat_ms() + 15000u; go(S_TIP); break;
    case S_TIP: if (ui.halt_pick == 1 && ui.sel == 0) jam_start(1); else go(S_SOS); break;
    case S_SOS_EXIT: if (ui.sel == 0) end_sos(0); else go(S_SOS); break;
    case S_SOS_DONE: go(S_MAIN); break;
    case S_MENU:
        if (ui.sel == 0) jam_start(0);
        else if (ui.sel == 1) { ui.rep_top = 0; go(S_REP); }
        else if (ui.sel == 2) go(S_HELP);
        else if (ui.sel == 3) { go(S_SLIP_CONFIRM); ui.sel = 1; }
        else if (ui.sel == 4) go(S_SETTINGS);
        else go(S_MAIN);
        break;
    case S_JAM_REC: if (ui.jam_n >= 1) jam_listen(); break;
    case S_JAM_RESULT:
        if (ui.jam_ok == ui.jam_n) after_lesson();
        else if (ui.sel == 0) { ui.jam_tries++; ui.jam_n = 0; go(S_JAM_REC); }
        else { jam_keep(); toast("KEPT IT.", "IT IS OUR VERSION NOW.", 2000u); after_lesson(); }
        break;
    case S_REP:
        if (sg.nriffs) {
            const sg_riff_t *r = &sg.riffs[sg.nriffs - 1 - ui.sel];
            seq_clear(); play_riff(r->notes, r->n, 0);
        }
        break;
    case S_HELP: go(S_GROUPS); break;
    case S_GROUPS: go(S_ABOUT_GROUPS); break;
    case S_SLIP_CONFIRM: if (ui.sel == 0) { sg_slip(); changed(1); go(S_SLIP_THANKS); } else go(S_MAIN); break;
    case S_SLIP_THANKS: go(S_SLIP_WHAT); break;
    case S_SLIP_WHAT: go(S_SLIP_WHO); break;
    case S_SLIP_WHO: go(S_SLIP_DONE); break;
    case S_SLIP_DONE: go(S_MAIN); break;
    case S_SETTINGS:
        if (ui.sel == 0) { sg.character = (uint8_t)((sg.character + 1) % CH_N); om_instrument(buddy_instrument()); changed(1); }
        else if (ui.sel == 1) { sg.thoughts_on = !sg.thoughts_on; changed(1); }
        else if (ui.sel == 2) {
            int v = proj.par[P_LEVEL] + 20;
            proj.par[P_LEVEL] = (int16_t)(v > 100 ? 40 : v); om_set(P_LEVEL, proj.par[P_LEVEL]); changed(1);
        } else go(S_MAIN);
        ui.redraw = 1;
        break;
    case S_LEVELUP:
        ui.levelup = 0;
        if (ui.sos_on) { om_instrument(INS_BELL); go(S_SOS); } else go(S_MAIN);
        break;
    default: break;
    }
    (void)n;
}
static void on_glo(void)
{
    switch (ui.scr) {
    case S_MAIN: go(S_MENU); break;
    case S_SOS: case S_HALT: case S_TIP: case S_SOS_EXIT: go(S_HELP); break;   /* help, the wave keeps time */
    case S_JAM_REC: case S_JAM_LISTEN: if (ui.jam_from_sos && ui.sos_on) { om_instrument(INS_BELL); go(S_SOS); } else go(S_MENU); break;
    case S_REP: case S_SETTINGS: go(S_MENU); break;
    case S_HELP: if (ui.sos_on) { go(S_SOS); break; } go(S_MENU); break;
    case S_GROUPS: go(S_HELP); break;
    case S_ABOUT_GROUPS: go(S_GROUPS); break;
    case S_MENU: case S_ABOUT_PRO: case S_STATUS: case S_STATUS2: case S_DONE_TODAY: case S_SLIP_CONFIRM: go(S_MAIN); break;
    default: break;
    }
}
static void on_home_tap(void)
{
    if (ui.scr == S_MAIN) go(S_STATUS);
    else if (ui.scr == S_STATUS) go(S_STATUS2);
    else if (ui.scr == S_STATUS2) go(S_MAIN);
}
static void on_home_hold(void)
{
    int c;
    if (ui.scr != S_MAIN && ui.scr != S_STATUS && ui.scr != S_STATUS2) return;
    c = sg_can_declare(plat_ms());
    if (c == 0) go(S_DONE_TODAY);
    else if (c == 2) go(S_NEWDAY);
    else declare();
}
static void on_black(int i)
{
    if (ui.scr == S_HALT && i < 4) { ui.sel = (uint8_t)i; on_sel(); return; }
    if (i == KEY_SOS) {
        if (ui.scr >= S_MAIN && !ui.sos_on && ui.scr != S_LEVELUP) start_sos();
        return;
    }
    if (ui.scr == S_MAIN && i < 4) care(i);
}
static void on_white(int wi, int key)
{
    if (ui.scr == S_JAM_REC) {
        if (ui.jam_n < SG_RIFF_NOTES) { ui.jam_notes[ui.jam_n++] = (uint8_t)wi; ui.redraw = 1; ui.jam_at = plat_ms(); }
    }
    om_trigger(key, 100);
}

/* ----------------------------------------------------------------- the loop --- */
void ui_init(void)
{
    uint32_t now = plat_ms();
    ui.keys = plat_keys(); ui.buttons = plat_buttons();
    ui.msg = 0; ui.msg_sub[0] = 0; ui.msg_until = now;
    ui.home_down = ui.sos_down = now; ui.home_fired = ui.sos_fired = 0;
    ui.care = CARE_NONE; ui.sos_on = 0; ui.nseq = 0; ui.levelup = 0;
    ui.last_change = ui.last_save = ui.last_check = now; ui.important = 0; ui.dirty_state = 0;
    ui.anim_at = now; ui.frame = 0; ui.rng ^= now * 2654435761u + 0x5bd1e995u;
    om_instrument(buddy_instrument());
    plat_glow(0); plat_play_red(0);
    go(S_SPLASH);
    lights();
}
static void autosave(uint32_t now)
{
    if (!ui.dirty_state || plat_keys()) return;
    if (ui.important ? now - ui.last_change < 800u : (now - ui.last_change < 5000u || now - ui.last_save < 60000u)) return;
    if (now - ui.last_check < 1000u) return;
    ui.last_check = now;
    if (!sg_needs_save()) { ui.dirty_state = 0; ui.important = 0; return; }
    if (sg_save() == 0) { ui.dirty_state = 0; ui.important = 0; ui.last_save = now; }
}
static void sos_tick(uint32_t now)
{
    uint32_t t;
    if (!ui.sos_on) return;
    t = now - ui.sos_start;
    if (now - ui.breath_at >= BREATH_MS / 2u) {
        ui.breath_at += BREATH_MS / 2u;
        ui.breath_in = !ui.breath_in;
        if (ui.breath_in) om_note(INS_BASS, 0, 70, 4600);    /* in with the bass, out in silence */
        if (ui.scr == S_SOS) ui.redraw = 1;
    }
    if (!ui.halt_asked && t >= HALT_AT_MS && ui.scr == S_SOS) { ui.halt_asked = 1; go(S_HALT); }
    if (ui.scr == S_TIP && (int32_t)(now - ui.tip_until) >= 0 && ui.halt_pick != 1) go(S_SOS);
    if (t >= SOS_MS && (ui.scr == S_SOS || ui.scr == S_HALT || ui.scr == S_TIP || ui.scr == S_SOS_EXIT)) end_sos(1);
}
static void jam_tick(uint32_t now)
{
    if (ui.scr == S_JAM_REC && ui.jam_n >= SG_RIFF_NOTES && now - ui.jam_at >= 600u) jam_listen();
    if (ui.scr == S_JAM_LISTEN) {
        uint32_t t = now - ui.jam_at;
        if (t >= 700u && ui.jam_played < ui.jam_n && t >= 700u + (uint32_t)ui.jam_played * 420u) {
            om_note(buddy_instrument(), ui.jam_try[ui.jam_played], 100, 340);
            ui.jam_played++; ui.redraw = 1;
        }
        if (ui.jam_played >= ui.jam_n && t >= 700u + (uint32_t)ui.jam_n * 420u + 900u) {
            if (ui.jam_ok == ui.jam_n) { jam_keep(); fanfare(); }
            go(S_JAM_RESULT);
        }
    }
}
static void input(void)
{
    uint32_t now = plat_ms(), keys = plat_keys(), btn = plat_buttons();
    uint32_t kp = keys & ~ui.keys, kr = ui.keys & ~keys, bp = btn & ~ui.buttons, br = ui.buttons & ~btn;
    int32_t enc = plat_enc(EN_SELECT);
    int k;
    ui.keys = keys; ui.buttons = btn;
    for (k = 0; k < NKEYS; k++) {
        if (kr >> k & 1u) om_note_off(k);
        if (kp >> k & 1u) {
            int wi = white_index_of(k), bi = black_index_of(k);
            if (wi >= 0) on_white(wi, k);
            else if (bi >= 0) on_black(bi);
        }
    }
    /* the SOS key held for 2 s during a wave asks to stop early */
    if (kp & SOS_KEY_BIT) { ui.sos_down = now; ui.sos_fired = 0; }
    if ((keys & SOS_KEY_BIT) && !ui.sos_fired && now - ui.sos_down >= SOS_HOLD_MS && ui.sos_on && ui.scr == S_SOS) {
        ui.sos_fired = 1; go(S_SOS_EXIT); ui.sel = 1;
    }
    if (enc) {
        int n = opt_count();
        if (ui.scr == S_MOOD) {
            int v = ui.mood_pick + (enc > 0 ? 1 : -1);
            ui.mood_pick = (uint8_t)(v < 1 ? 1 : v > 5 ? 5 : v); ui.redraw = 1;
        } else if (n > 0) {
            int v = (int)ui.sel + (enc > 0 ? 1 : -1);
            ui.sel = (uint8_t)(v < 0 ? 0 : v >= n ? n - 1 : v);
            if (ui.scr == S_REP) {
                if (ui.sel < ui.rep_top) ui.rep_top = ui.sel;
                if (ui.sel >= ui.rep_top + 6) ui.rep_top = (uint8_t)(ui.sel - 5);
            }
            ui.redraw = 1;
        }
    }
    if (bp & (1u << B_SEL)) on_sel();
    if (bp & (1u << B_GLO)) on_glo();
    if (bp & (1u << B_SAVE)) {
        changed(1); ui.last_check = 0;
        if (sg_save() == 0) { ui.dirty_state = 0; ui.important = 0; ui.last_save = now; toast("SAVED.", 0, 1200u); }
        else toast("SAVE FAILED.", "WILL RETRY.", 1500u);
    }
    if (bp & (1u << B_HOME)) { ui.home_down = now; ui.home_fired = 0; }
    if ((btn & (1u << B_HOME)) && !ui.home_fired && now - ui.home_down >= HOME_HOLD_MS) { ui.home_fired = 1; on_home_hold(); }
    if ((br & (1u << B_HOME)) && !ui.home_fired) on_home_tap();
    seq_tick(now);
    sos_tick(now);
    jam_tick(now);
    sg_tick(now);
    autosave(now);
    if (ui.scr == S_SPLASH && now - ui.entered >= 3000u) go(sg.onboarded ? S_MAIN : S_WHAT);
}
void ui_input_only(void) { input(); }
void ui_frame(void)
{
    uint32_t now = plat_ms();
    input();
    if (now - ui.anim_at >= ANIM_MS) {
        ui.anim_at = now; ui.frame ^= 1u;
        if (ui.scr == S_MAIN || ui.scr == S_SOS || ui.scr == S_P_KNOCK || ui.scr == S_JAM_LISTEN || ui.scr == S_SLIP_THANKS)
            ui.redraw = 1;
    }
    if (ui.msg && ui.msg_until && (int32_t)(now - ui.msg_until) >= 0) { ui.msg = 0; ui.msg_until = 0; ui.redraw = 1; }
    if (ui.care && (int32_t)(now - ui.care_until) >= 0) { ui.care = CARE_NONE; ui.redraw = 1; }
    if (ui.scr == S_SOS && (now - ui.sos_start) / 1000u != (now - 16u - ui.sos_start) / 1000u) ui.redraw = 1;
    if (ui.redraw) { ui.redraw = 0; render(); lights(); }
}
