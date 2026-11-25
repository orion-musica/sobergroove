/* SPDX-License-Identifier: GPL-3.0-only */
/* Sober Groove pixel art, in the style of an old phone's LCD: a 32 x 26 grid of
 * dots composed into row bitmasks, then drawn as squares of any size. Original art. */
#pragma once
#define LCD_BG RGB(169,189,139)
#define LCD_INK RGB(30,43,22)
#define LCD_DIM RGB(146,166,118)
#define GRID_W 32
#define GRID_H 26
#define NROWS(a) ((int)(sizeof(a) / sizeof((a)[0])))
typedef const char *const bitmap_t[];

static bitmap_t SPR_LOW = {
 "....................", ".....######......##.", "....########.....#..", "....#......#....#...",
 "....#.#..#.#...#....", "....#......#..#.....", "....#..##..#.#......", ".....######.#.......",
 "...#########........", "..#........##.......", "..#.......####......", "..#......######.....",
 "..#......##..##.....", "..#......######.....", "..#.......####......", "..##########........",
 "...#.....#..........", "...#.....#..........", "..##.....##........."};
static bitmap_t SPR_KIT = {
 ".#................#.", "..#....######....#..", "...#..########..#...", "....#.#......#.#....",
 "....#.#.#..#.#.#....", "....#.#......#.#....", "....#.#..##..#.#....", "....#..######..#....",
 ".....##########.....", ".......#....#.......", ".......#....#.......", "..################..",
 "..#..............#..", "..#..#..#..#..#..#..", "..#..............#..", "..################..",
 "...#............#...", "..#..............#.."};
static bitmap_t SPR_VINYL = {
 "....................", "......########......", ".....#........#.....", "....###......###....",
 "....###.#..#.###....", "....###......###....", ".....#..#..#..#.....", "......#..##..#......",
 ".......######.......", ".....##########.....", "....#..........#....", "....#..........#....",
 ".##################.", ".#...######....#..#.", ".#..##....##....#.#.", ".#..##.##.##......#.",
 ".#..##....##......#.", ".#...######.......#.", ".##################."};
static const int8_t EYES[3][2] = {{6, 9}, {8, 11}, {8, 11}};     /* eye columns, row 4 */

static bitmap_t HEAD_LOW = {".######.", "########", "#......#", "#.##.###", "#......#", "#..##..#", ".######."};
static bitmap_t HEAD_KIT = {".######.##", "########.#", "#......#..", "#.##.###..", "#......#..", "#..##..#..", ".######..."};
static bitmap_t HEAD_VINYL = {"..########..", ".#........#.", "###......###", "###.##.#####", "###......###",
                              ".#........#.", "..#.####.#..", "...######..."};
static bitmap_t BODY = {".##########.", "#..........#", "#.########.#", "#.#......#.#", ".##......##.", "..#......#..", ".##......##."};
static bitmap_t COUCH = {".#..........#.", "##############", "#............#", "##############", "#............#"};

static bitmap_t BM_NOTE = {"..##.", "..#.#", "..#..", ".##..", "###..", ".#..."};
static bitmap_t BM_Z = {"####", "..#.", ".#..", "####"};
static bitmap_t BM_ZS = {"###", ".#.", "###"};
static bitmap_t BM_BOWL = {"#######", "#.....#", ".#...#.", "..###.."};
static bitmap_t BM_STEAM = {".#...#.", "..#.#..", ".#...#."};
static bitmap_t BM_HEART = {".#.#.", "#####", ".###.", "..#.."};
static bitmap_t BM_SUN = {".#.#.#.", "..###..", "#.###.#", "..###..", ".#.#.#."};
static bitmap_t BM_MOON = {".###.", "##...", "#....", "##...", ".###."};
static bitmap_t BM_CLOUD = {"..###...", ".#####..", "########"};
static bitmap_t BM_RAIN = {".#..#..#"};
static bitmap_t BM_RAIN2 = {"#..#..#."};
static bitmap_t BM_MIC = {".#.", "###", "###", ".#.", ".#.", ".#.", ".#.", ".#.", "###"};
static bitmap_t BM_SPARK = {"..#..", "..#..", "##.##", "..#..", "..#.."};
static bitmap_t BM_STAR = {".#.", "#.#", ".#."};
static bitmap_t BM_Q = {"###", "..#", ".##", "...", ".#."};
static bitmap_t BM_DUST = {"#.#..............#.#"};
static bitmap_t BM_PHONE = {"####", "#..#", "#..#", "#..#", "####", "#.##", "####"};
static bitmap_t BM_HAT = {"..#..", ".###.", "#####"};
static bitmap_t BM_AMP = {"######", "#....#", "#.##.#", "#.##.#", "#....#", "######", "#....#"};
static bitmap_t BM_POSTER = {"#..#", "####", "#..#", "####"};
static bitmap_t BM_LAMPTOP = {"###", ".#."};
static bitmap_t BM_DOOR = {"######", "#....#", "#....#", "#....#", "#....#", "#....#", "#...##", "#....#",
                           "#....#", "#....#", "#....#", "#....#", "#....#", "#....#", "#....#", "#....#",
                           "#....#", "#....#", "#....#", "######"};
static bitmap_t BM_KNOCK = {"#.#", ".#.", "#.#"};
static bitmap_t BM_CROWD = {"#....#....#....#....#....#....#.", ".##..##..##..##..##..##..##..##.",
                            "####.####.####.####.####.####.##", "####.####.####.####.####.####.##"};
static bitmap_t BM_TRUSS = {"#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#."};
static bitmap_t BM_FLOOR = {"#.#.#.#.#.#.#.#.#.#.#.#.#.#.#.#."};
static bitmap_t BM_SOLID = {"################################"};
static bitmap_t BM_STAGE_LEGS = {"#...#...#...#...#...#...#...#..."};
static bitmap_t BM_RAINFIELD = {"#...#...#...#...#...#...", "", "..#...#...#...#...#...#."};

static uint32_t px_rows[GRID_H];
static void px_clear(void) { int i; for (i = 0; i < GRID_H; i++) px_rows[i] = 0; }
static void px_dot(int x, int y)
{
    if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) px_rows[y] |= 1u << x;
}
static void px_unset(int x, int y)
{
    if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) px_rows[y] &= ~(1u << x);
}
static void px_bm(const char *const *rows, int n, int ox, int oy)
{
    int x, y;
    for (y = 0; y < n; y++)
        for (x = 0; rows[y][x]; x++)
            if (rows[y][x] == '#') px_dot(x + ox, y + oy);
}
#define PX(bm, x, y) px_bm(bm, NROWS(bm), x, y)
/* sin over a 16-step period, x100 */
static const int8_t SIN16[16] = {0, 38, 71, 92, 100, 92, 71, 38, 0, -38, -71, -92, -100, -92, -71, -38};
static void px_wave(int amp, int phase)
{
    int x;
    for (x = 0; x < GRID_W; x++) {
        int v = amp * SIN16[(x + phase) & 15];
        px_dot(x, 2 - (v >= 0 ? (v + 50) / 100 : -((-v + 50) / 100)));
    }
}
static void px_sprite(int ch, int ox, int oy, int closed)
{
    const char *const *rows = ch == CH_KIT ? SPR_KIT : ch == CH_VINYL ? SPR_VINYL : SPR_LOW;
    int n = ch == CH_KIT ? NROWS(SPR_KIT) : ch == CH_VINYL ? NROWS(SPR_VINYL) : NROWS(SPR_LOW);
    px_bm(rows, n, ox, oy);
    if (closed) {
        int e;
        for (e = 0; e < 2; e++) { px_dot(ox + EYES[ch][e], oy + 4); px_dot(ox + EYES[ch][e] + 1, oy + 4); }
    }
}
/* stage 1: the buddy slumped on your couch */
static void px_slumped(int ch, int dy)
{
    if (ch == CH_KIT) PX(HEAD_KIT, 11, 6 + dy);
    else if (ch == CH_VINYL) PX(HEAD_VINYL, 9, 6 + dy);
    else PX(HEAD_LOW, 11, 6 + dy);
    PX(BODY, 9, 13 + dy);
    PX(COUCH, 8, 20);
    PX(BM_SOLID, 0, 25);
}
/* the buddy where the stage puts it; dy = the idle bob */
static void px_stage_scene(int stage, int ch, int closed, int dy)
{
    switch (stage) {
    case 1: px_slumped(ch, dy); break;
    case 2: px_sprite(ch, 6, 5 + dy, closed); PX(BM_HAT, 0, 22); PX(BM_LAMPTOP, 28, 2);
            { int y; for (y = 4; y < 25; y++) px_dot(29, y); } PX(BM_FLOOR, 0, 25); break;
    case 3: px_sprite(ch, 9, 5 + dy, closed); PX(BM_AMP, 0, 17); PX(BM_POSTER, 0, 1); PX(BM_SOLID, 0, 25); break;
    case 4: px_sprite(ch, 6, 3 + dy, closed); PX(BM_SOLID, 0, 22); PX(BM_STAGE_LEGS, 0, 23);
            PX(BM_LAMPTOP, 28, 0); px_dot(27, 3); px_dot(25, 5); px_dot(30, 4); px_dot(29, 7); break;
    default: PX(BM_TRUSS, 0, 0); px_sprite(ch, 6, 1 + dy, closed);
             px_dot(2, 2); px_dot(29, 2); px_dot(3, 4); px_dot(28, 4); px_dot(4, 6); px_dot(27, 6);
             PX(BM_SOLID, 0, 20); px_bm(BM_CROWD, 1, 0, 21 + (dy & 1)); px_bm(BM_CROWD + 1, 1, 0, 22);
             px_bm(BM_CROWD + 2, 2, 0, 23); break;
    }
}
static void px_door(int ch, int knock)
{
    px_sprite(ch, 2, 6, 0);
    PX(BM_DOOR, 26, 5); PX(BM_SOLID, 0, 25);
    px_bm(BM_RAINFIELD, 1, 0, 0); px_bm(BM_RAINFIELD + 2, 1, 0, 2);
    if (knock) { PX(BM_KNOCK, 22, 9); PX(BM_KNOCK, 22, 14); }
}
/* draw the grid at (x0, y0), each dot a size x size square */
static void px_draw(int x0, int y0, int size, uint16_t ink)
{
    int x, y;
    for (y = 0; y < GRID_H; y++) {
        uint32_t r = px_rows[y];
        if (!r) continue;
        for (x = 0; x < GRID_W; x++)
            if (r >> x & 1u) cv_rect(x0 + x * size, y0 + y * size, size - (size > 3 ? 1 : 0), size - (size > 3 ? 1 : 0), ink);
    }
}
