# Content guide

How Sober Groove talks. Every sentence on the device lives in `firmware/src/app/sg_text.h` (and a few short status lines in `ui.c`). Use this guide for new wording and for review.

## Who reads it

Musicians, DJs and producers in the first weeks and months without alcohol. Often tired, sometimes ashamed, often at night after a gig. They read the screen at a glance on a 1.54-inch display.

## Voice

- **Warm, short, concrete.** "I'm here. We'll ride it out." Not "Remember that you are not alone in your journey."
- **The buddy is a peer, not an expert.** It wonders and shares; it does not instruct or diagnose. "I read that…", "Wonder if…", "Maybe I will…".
- **Second person for the user, first person for the buddy.**
- **Calm in hard moments.** No exclamation marks during SOS or after a slip. A fanfare is fine after a wave is ridden.
- **English on the device** for now; Polish is planned. Keep wording simple enough to translate.

## Never

- Judgement or failure language: "you failed", "start over", "back to zero", "you lost", "relapse" for a single slip.
- Shame or guilt hooks: the buddy is never hurt, sick, angry or disappointed because of the user.
- Comparing whose problem is "bigger" or "smaller".
- Alcohol imagery or jokes: no bottles, glasses, bars, "one more round".
- Diagnoses, medication names, doses or advice about withdrawal, except "talk to a GP".
- Phone numbers in firmware: they change. Name the organisation; numbers belong in the README and on the organisations' sites, checked at each release.
- Promises: "this will cure", "guaranteed", "you will never…".
- Pressure towards any one path. Therapy, groups and AA appear as interesting options, never as orders.

## Thoughts

At most one a day, after the morning hold, never during SOS.

| Kind | Share | Purpose |
| --- | --- | --- |
| `TH_REFLECT` | about 60% | Curiosity and simple, checked facts about cravings, sleep, routines; ties to music |
| `TH_PEOPLE` | about 25% | Friends, bandmates, support groups (therapy groups, Music Support groups, SMART Recovery; AA only as one option) |
| `TH_PRO` | about 15% | A therapist as "a producer for your head", "a fresh ear on a mix". At most once a week, never twice in a row |

Each thought is a wondering, not a recommendation. Facts must come from reliable sources (NHS, NIAAA, recovery organisations) and the source is noted in the pull request.

## The story

- The buddy lost the band and the flat and lives on your couch. Never describe the fall in detail: no violence, no nights on the street, no drinking scenes. "I kept turning up late. Then not at all." is the limit.
- Reconciliation with the old band is optional and uncertain: trying is enough.
- The buddy never leaves.
- A slip does not undo the story: "Still here, still on your couch."

## Format

| Element | Limit |
| --- | --- |
| A line in the bold face (`FONT_B`) | 26 characters |
| A hint line (`FONT_XS`) | about 36 characters |
| Lines per page | 4–6, less when there is art |
| Case | Upper case on the device |

## Review

All wording is a draft until reviewed by an addiction professional. A pull request that changes `sg_text.h` should say what changed and why, and list sources for any facts. Wording that touches safety (withdrawal, crisis, help) needs a second reviewer.
