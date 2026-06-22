#ifndef SHUNT_H
#define SHUNT_H

#include <emscripten/emscripten.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* ── Config ── */
#define WIDTH 1280
#define HEIGHT 720
#define PIXEL(X, Y) (((Y) * (WIDTH) + (X)) * 4)

#define GAME_W 20
#define GAME_H 20
#define NUM_TILES GAME_W *GAME_H

// Isometric projection: 2:1
// UP -> top-left (toward 0,0), DOWN -> bottom-right.
#define HX 32           // half tile width  (2 ...)
#define HY 16           // half tile height (... 1)
#define TILE_W (HX * 2) // sprite art size: 64
#define TILE_H (HY * 2) // 32
#define OX 0            // board offset within the framebuffer
#define OY 360
#define SWITCH_HIT 32   // half-size of a switch's square click zone, screen px

#define SUBTILE 100                // virtual units per tile edge
#define WORLD_W (GAME_W * SUBTILE) // 2000
#define WORLD_H (GAME_H * SUBTILE)
#define SPEED 5 // world units per tick (100u = 50px, so 2u ≈ the old 1px/tick)

#define ORIGIN_COL 10
#define ORIGIN_ROW 1

#define FPS 60
#define MAX_BOXES 10
#define DELAY 3 * FPS

#define SPRITE_MAX (64 * 64 * 4) // generous per-sprite buffer

/* ── Types ── */
typedef struct
{
    uint8_t type;
    uint8_t dir;
    uint16_t centre_x;
    uint16_t centre_y;
    uint8_t colour;    // exits: which colour box this exit accepts
    uint8_t capacity;  // exits: how many boxes this exit needs
    uint8_t dirs_mask; // switches: bitmask of allowed dirs, bit = (1 << Dir)
} Tile;

typedef struct
{
    int x;
    int y;
    uint8_t colour;
    bool active;
} Box;

typedef struct {
    uint8_t rgba[SPRITE_MAX];
    int w, h;
} Sprite;

typedef struct {
    int x, y;
} Pt;

typedef struct {
    uint8_t r, g, b;
} RGB;

typedef enum {
    COLOUR_RED,
    COLOUR_BLUE,
    COLOUR_GREEN,
    COLOUR_YELLOW,
    COLOUR_WHITE,
    COLOUR_COUNT // final index is the number of actual colours
} Colour;

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT
} Dirs;

typedef enum {
    TILE_CONVEYOR,
    TILE_ENTRY,
    TILE_EXIT,
    TILE_SWITCH,
    TILE_INACTIVE
} Tile_Type;

// Sprite ids: a tile id is SPR_TILE_UP + Dirs, a box id is SPR_BOX_RED + Colour,
// a switch id is SPR_SWITCH_UP + Dirs — selection is a base plus the enum value.
typedef enum {
    SPR_TILE_UP,
    SPR_TILE_RIGHT,
    SPR_TILE_DOWN,
    SPR_TILE_LEFT, // align with Dirs
    SPR_BOX_RED,
    SPR_BOX_BLUE,
    SPR_BOX_GREEN,
    SPR_BOX_YELLOW,
    SPR_BOX_WHITE, // align with Colour
    SPR_SWITCH_UP,
    SPR_SWITCH_RIGHT,
    SPR_SWITCH_DOWN,
    SPR_SWITCH_LEFT, // align with Dirs
    SPR_COUNT
} SpriteId;

#endif // SHUNT_H
