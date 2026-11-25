/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove words. Short lines for a 240-pixel screen (at most 26 characters in the
 * bold face). Tone: warm, calm, no judgement. Reviewed wording belongs here, not in code,
 * so a therapist can read and change it without touching the logic. */
#pragma once

/* ---- first start */
static const char *const TXT_WHAT[] = {
    "A BUDDY WHO HELPS YOU", "STAY SOBER:", "", "- COUNTS SOBER DAYS", "- HELPS RIDE CRAVINGS",
    "- LEARNS MUSIC FROM YOU", 0};
static const char *const TXT_SAFETY[] = {
    "THIS IS SUPPORT,", "NOT TREATMENT.", "", "STOPPING HEAVY DRINKING", "SUDDENLY CAN BE", "DANGEROUS.",
    "TALK TO A GP FIRST.", 0};
static const char *const TXT_READY[] = {"EVERY MORNING HOLD HOME", "AND SAY: NOT TODAY.", "", "BLACK KEY 5 = SOS", 0};

/* ---- prologue (after choosing the buddy) */
static const char *const TXT_WHO[3][5] = {
    {"HEY. I AM LOW.", "THEY KICKED ME OUT", "OF THE BAND. THEN", "OUT OF THE FLAT.", 0},
    {"HI. I AM KIT.", "I LOST MY PLACE IN", "THE BAND. AND MY ROOM", "WENT WITH IT.", 0},
    {"YO. I AM VINYL.", "NO MORE RESIDENCY,", "NO FLAT ABOVE THE CLUB.", "JUST MY RECORDS.", 0},
};
static const char *const TXT_WHY[] = {"I KEPT TURNING UP LATE.", "THEN NOT AT ALL.", 0};
static const char *const TXT_CRASH[] = {"CAN I CRASH HERE", "FOR A BIT?", 0};
static const char *const TXT_THANKS[] = {"THANKS. I WILL TRY", "NOT TO MESS THIS UP.", 0};
static const char *const TXT_SAMEROAD[] = {"YOU'VE GOT YOUR OWN", "STUFF TOO, I KNOW.", "SAME ROAD.", "WE'LL FIGURE IT OUT.", 0};

/* ---- help: no numbers in firmware; they are checked and published with each release */
static const char *const TXT_HELP[] = {
    "MUSIC SUPPORT (UK)", " MON-FRI 9-17", "SUPPORT GROUPS: SEL", "SAMARITANS 24/7", "YOUR GP",
    "SOMEONE YOU TRUST", 0};
static const char *const TXT_GROUPS[] = {
    "THERAPY GROUPS", " LED BY A PRO", "MUSIC SUPPORT GROUPS", " FOR MUSIC PEOPLE", "SMART RECOVERY", "AA", 0};
static const char *const TXT_ABOUT_PRO[] = {
    "YOU TALK, THEY LISTEN.", "NO GRADES,", "NO JUDGEMENT.", "", "START WITH YOUR GP", "OR MUSIC SUPPORT.", 0};
static const char *const TXT_ABOUT_GROUPS[] = {
    "PEOPLE ON THE SAME", "ROAD, TALKING.", "SOME ARE LED BY A PRO,", "SOME ARE JUST FOR", "MUSIC PEOPLE.", 0};

/* ---- the buddy's thoughts: mostly curiosity, then people and groups, rarely a pro */
enum { TH_REFLECT, TH_PEOPLE, TH_PRO };
typedef struct { uint8_t kind; const char *line[5]; } thought_t;
static const thought_t THOUGHTS[] = {
    {TH_REFLECT, {"CRAVINGS COME LIKE", "WAVES, I READ.", "FUNNY. GOOD GROOVES", "DO TOO.", 0}},
    {TH_REFLECT, {"WHY DO I MISS IT MOST", "AFTER GIGS?", "MAYBE IT IS THE SILENCE", "AFTER THE NOISE.", 0}},
    {TH_PEOPLE, {"SOMEONE FROM THE SCENE", "QUIT DRINKING YEARS AGO.", "MAYBE I WILL ASK HOW", "MONTH ONE WENT.", 0}},
    {TH_REFLECT, {"I SLEEP BADLY LATELY.", "WONDER IF THAT IS", "PART OF THE DEAL", "EARLY ON.", 0}},
    {TH_PEOPLE, {"HEARD THERE ARE", "RECOVERY GROUPS FOR", "MUSIC PEOPLE. LIKE A", "BAND, BUT FOR SOBER.", 0}},
    {TH_REFLECT, {"SOMEONE WROTE THAT A", "CRAVING IS THE BRAIN,", "NOT WEAK WILL.", "THAT IS A RELIEF.", 0}},
    {TH_PRO, {"SOME MUSICIANS HAVE A", "PRODUCER FOR THEIR HEAD.", "A THERAPIST. SOUNDS", "KIND OF INTERESTING.", 0}},
    {TH_REFLECT, {"CRAVINGS GET WEAKER", "THE LONGER YOU DON'T", "FEED THEM, THEY SAY.", "I HOPE SO.", 0}},
    {TH_PEOPLE, {"SOME GROUPS HAVE A", "THERAPIST. ALL TALK,", "SOMEONE KEEPS TEMPO.", "INTERESTING.", 0}},
    {TH_REFLECT, {"PLAYED WITH YOUR RIFF", "ALL NIGHT IN MY HEAD.", "BETTER THAN WHAT USED", "TO KEEP ME UP.", 0}},
    {TH_PEOPLE, {"PLAYING ALONE IS FINE.", "PLAYING WITH OTHERS", "TEACHES OTHER THINGS.", "RECOVERY TOO?", 0}},
    {TH_REFLECT, {"STAGE FRIGHT OR", "SOMETHING MORE?", "CAN'T TELL YET.", "I'LL KEEP LISTENING.", 0}},
    {TH_PRO, {"WONDER WHAT A PRO", "WOULD HEAR IN MY", "THOUGHTS. LIKE A FRESH", "EAR ON A MIX.", 0}},
    {TH_REFLECT, {"A SLIP IS NOT THE SAME", "AS GOING BACK.", "I READ THAT. I WANT", "TO BELIEVE IT.", 0}},
};
#define NTHOUGHTS ((int)(sizeof(THOUGHTS) / sizeof(THOUGHTS[0])))

/* ---- SOS */
static const char *const HALT_ASK[4] = {"HUNGRY", "ANGRY", "LONELY", "TIRED"};
static const char *const HALT_TIP[4][4] = {
    {"EAT SOMETHING NOW.", "HUNGER CAN FEEL", "LIKE A CRAVING.", 0},
    {"PLAY IT OUT.", "HIT THE KEYS HARD,", "THEN BREATHE.", 0},
    {"CALL SOMEONE NOW.", "OR TEXT SOMEONE.", "GLO OPENS HELP.", 0},
    {"REST, DON'T DRINK.", "LIE DOWN FOR", "TEN MINUTES.", 0},
};

/* ---- story chapters, one per stage (2..5), Kit's old bandmate is a singer */
static const char *const TXT_CHAPTER[SG_STAGES + 1][4] = {
    {0}, {0},
    {"PLAYED ON THE STREET.", "YOUR RIFFS WOULD NOT", "LEAVE ME ALONE.", 0},
    {"FOUND A REHEARSAL ROOM.", "THINKING ABOUT THE OLD", "BAND SOMETIMES.", 0},
    {"FIRST SOBER GIG.", "THINKING OF TEXTING", "OUR OLD DRUMMER. MAYBE.", 0},
    {"YOU LET ME IN WHEN", "I HAD NOWHERE TO GO.", "THANK YOU.", 0},
};
static const char *const TXT_CHAPTER4_KIT[] = {"FIRST SOBER GIG.", "THINKING OF TEXTING", "OUR OLD SINGER. MAYBE.", 0};

/* ---- slip */
static const char *const SLIP_TRIGGERS[] = {"STRESS", "PEOPLE", "A GIG", "BEING ALONE", "OTHER", 0};
static const char *const SLIP_WHO[] = {"SOMEONE I TRUST", "A HELPLINE", "MY GROUP", "NOT SURE YET", 0};
