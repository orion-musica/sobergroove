# Sober Groove 0.1.0-alpha

The first test version of Sober Groove, a recovery companion for musicians on the M-VAVE FM-1.

## In this alpha

- **First start:** a splash that says what it is for, what it does, a safety note (support, not treatment; see a GP before stopping heavy drinking), choosing **Low**, **Kit** or **Vinyl**, the prologue (the knock at the door, "Can I crash here for a bit?", the couch) and where to find help.
- **Mornings:** hold HOME to say "not today". Total sober days, streak and best streak. After a restart the buddy asks whether it is a new day; the same day never counts twice.
- **Care** on black keys 1–4: meal, talk (mood 1–5), move, sleep/wake. Energy, mood and rhythm drift slowly and never fall below a floor. Nothing dies.
- **SOS** on black key 5: a 20-minute wave with breathing at six breaths a minute (in with the bass, out in silence), play-along bell, HALT check at five minutes with a tip for each answer, a halfway reminder, help from GLO without stopping the wave, early exit with no penalty, and a fanfare for a wave ridden out.
- **Lessons (Jam):** teach up to four notes or hits; the buddy listens and plays them back, with mistakes that become rarer as it grows. Teach again or keep its version. When "angry" in HALT, you can teach a riff to play it out.
- **Repertoire:** every riff, newest first, with the day it was learned; play any of them back.
- **Five stages** needing both days and lessons: LOST (3 days + 3 lessons) → STREET → GARAGE (14 + 10) → CLUB (30 + 20) → MAIN STAGE (90 + 40). A stage never falls. Each stage up shows a short chapter of the story.
- **Slip:** confirm (defaults to no), "a slip is not the end", what happened, who can help today; the streak restarts, everything else stays.
- **Thoughts:** at most one a day after the morning: mostly curiosity, sometimes people and support groups, a therapist at most once a week, always as an interesting path. Can be turned off.
- **Help:** Music Support, therapy groups, SMART Recovery, Samaritans, GP, AA as one option. No phone numbers in firmware.
- **Settings:** buddy, thoughts on/off, volume. Changing the buddy keeps all progress.
- **Sound:** FM bass (Low), synthesized drums (Kit), pluck (Vinyl), soft bell; no samples.
- **Saving:** automatic and with SAVE, one storage object with A/B copies and CRC.

## Not yet / known limits

- **Not yet built with the JieLi toolchain or tested on a device.** No `.fwsc` in this release; build it yourself. Please report how the build and install go.
- No real-time clock: days are counted by the morning hold and the "new day?" question.
- Lessons are the echo type for all three buddies. Planned: groove, fill and tempo ladder for Kit; drop on the one, beatmatching and phrase counting for Vinyl; root on the one and octave jumps for Low.
- Planned: Gig mode (before/after a concert, Afterglow), sober session counter, the web companion (date sync over USB MIDI, triggers, money saved, MIDI export of the buddy's own track), Polish language, a browser preview.
- All wording is a draft for review by an addiction professional before a wider release.

Firmware identity: **FM-1_506370001**.
