#include "shunt.h"

// Shunt — core. The browser owns the clock; C owns the rules.
//   init(seed)        start a run at the page-chosen framebuffer size
//   tick()            advance the simulation by exactly one step
//   click(x, y)       feed one input (framebuffer pixel coords)
//   framebuffer()     RGBA bytes the browser blits; fb_width()/fb_height() report size
//
// There is no time in here, ticks provided by browser.
// Config and types live in shunt.h; levels in levels.inc (included below).

static uint8_t fb[WIDTH * HEIGHT * 4]; // RGBA

// ── State: everything the simulation *is*. Mutated only by tick() and click().
static struct
{
    uint32_t tick;
    uint16_t delay; // gate to stop the next box appearing
    int level;
    bool cleared; // true once all levels are done
} S;

EMSCRIPTEN_KEEPALIVE int fb_width(void) { return WIDTH; }
EMSCRIPTEN_KEEPALIVE int fb_height(void) { return HEIGHT; }
EMSCRIPTEN_KEEPALIVE uint8_t *framebuffer(void) { return fb; } // address in wasm memory

// Sprites: JS decodes each PNG and writes its RGBA into sprites[id].rgba at
// load, then reports its size (the write-direction mirror of framebuffer()).
static Sprite sprites[SPR_COUNT];

EMSCRIPTEN_KEEPALIVE uint8_t *sprite_ptr(int id) { return sprites[id].rgba; }
EMSCRIPTEN_KEEPALIVE void set_sprite_size(int id, int w, int h) {
    sprites[id].w = w;
    sprites[id].h = h;
}

void draw_background();
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b);
void draw_tile(Tile tile, int col, int row);
void draw_boxes();
int get_grid(Box box);

static void load_level(int n);
static void start_level(int n);
static void blit_sprite(int id, int dx, int dy);
static void box_reached_exit(int i, Tile *exit);

static Tile tiles[GAME_W * GAME_H];
static Box box[MAX_BOXES];

static int tile_centre(int t) { return t * SUBTILE + SUBTILE / 2; } // grid  -> world centre
static int world_to_tile(int w) { return w / SUBTILE; }             // world -> grid

static Pt project(int wx, int wy) {
    return (Pt){(wx + wy) * HX / SUBTILE + OX, (wy - wx) * HY / SUBTILE + OY};
}

// step v toward target by up to SPEED, never overshooting — keeps == exact
static int approach(int v, int target) {
    int d = target - v;
    if (d > SPEED)
        return v + SPEED;
    if (d < -SPEED)
        return v - SPEED;
    return target;
}

static bool level_complete(void) {
    bool has_exit = false;
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type == TILE_EXIT) {
            has_exit = true;
            if (tiles[i].capacity > 0)
                return false;
        }
    }
    if (!has_exit)
        return false;
    for (int i = 0; i < MAX_BOXES; i++) {
        if (box[i].active)
            return false;
    }
    return true;
}

static void start_level(int n) {
    for (int i = 0; i < MAX_BOXES; i++)
        box[i].active = false;
    S.delay = 0;
    load_level(n);
}

EMSCRIPTEN_KEEPALIVE void init(uint32_t seed) {
    srand(seed);
    S.level = 1;
    S.cleared = false;
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].centre_x = tile_centre(i % GAME_W);
        tiles[i].centre_y = tile_centre(i / GAME_W);
    }
    start_level(1);
}

static void box_reached_exit(int i, Tile *exit) {
    box[i].active = false;

    if (exit->capacity == 0 || box[i].colour != exit->colour) {
        // Failure
        return;
    }
    // Success
    exit->capacity -= 1;
    exit->colour = rand() % COLOUR_COUNT;
}

// Pick a colour for the next box from current exits
static int pick_box_colour(void) {
    int want[COLOUR_COUNT] = {0};
    // Count demand
    for (int t = 0; t < NUM_TILES; t++) {
        if (tiles[t].type == TILE_EXIT && tiles[t].capacity > 0) {
            want[tiles[t].colour] += 1;
        }
    }
    // Remove supply
    for (int i = 0; i < MAX_BOXES; i++) {
        if (box[i].active) {
            want[box[i].colour] -= 1;
        }
    }

    int total = 0; // size of the bag
    for (int c = 0; c < COLOUR_COUNT; c++) {
        total += want[c];
    }
    if (total == 0) {
        return -1;
    }

    // Roulette wheel algorithm
    int r = rand() % total;
    for (int c = 0; c < COLOUR_COUNT; c++) {
        if ((r -= want[c]) < 0) {
            return c;
        }
    }
    return -1; // unreachable return
}

static void update(void) {
    S.tick++;
    if (S.delay < 1) {
        int colour = pick_box_colour();
        for (int i = 0; colour >= 0 && i < MAX_BOXES; i++) {
            if (box[i].active) {
                continue;
            }
            box[i].x = tile_centre(ORIGIN_COL);
            box[i].y = tile_centre(ORIGIN_ROW);
            box[i].active = 1;
            box[i].colour = colour;
            S.delay = DELAY;
            break;
        }
    }
    if (S.delay > 0) {
        S.delay -= 1;
    }

    for (int i = 0; i < MAX_BOXES; i++) {
        int grid = get_grid(box[i]);
        Tile this_tile = tiles[grid];

        if (this_tile.type == TILE_EXIT && box[i].active) {
            box_reached_exit(i, &tiles[grid]);
            continue;
        }

        int dir = this_tile.dir;
        int tile_x = this_tile.centre_x;
        int tile_y = this_tile.centre_y;

        // Ensure orthogonal alignment with belt before moving the "correct" direction
        if ((dir == UP || dir == DOWN) && box[i].x != tile_x) {
            box[i].x = approach(box[i].x, tile_x);
            continue;
        }
        if ((dir == LEFT || dir == RIGHT) && box[i].y != tile_y) {
            box[i].y = approach(box[i].y, tile_y);
            continue;
        }
        if (dir == UP)
            box[i].y = (box[i].y - SPEED + WORLD_H) % WORLD_H; // mod doesn't like negative numbers
        if (dir == RIGHT)
            box[i].x = (box[i].x + SPEED) % WORLD_W;
        if (dir == DOWN)
            box[i].y = (box[i].y + SPEED) % WORLD_H;
        if (dir == LEFT)
            box[i].x = (box[i].x - SPEED + WORLD_W) % WORLD_W;
    }

    if (!S.cleared && level_complete()) {
        S.level++;
        start_level(S.level);
        // If the new level has no exits, load_level was a no-op (past the end)
        bool has_exits = false;
        for (int i = 0; i < NUM_TILES; i++) {
            if (tiles[i].type == TILE_EXIT && tiles[i].capacity > 0) {
                has_exits = true;
                break;
            }
        }
        if (!has_exits)
            S.cleared = true;
    }
}

// Paint into the framebuffer
static void render(void) {
    draw_background();
    draw_boxes();
}

EMSCRIPTEN_KEEPALIVE void tick(void) {
    update();
    render();
}

EMSCRIPTEN_KEEPALIVE void clear_tiles(void) {
    int origin = ORIGIN_COL + ORIGIN_ROW * GAME_W;
    for (int i = 0; i < NUM_TILES; i++) {
        if (i == origin)
            continue;
        if (tiles[i].type == TILE_CONVEYOR && tiles[i].capacity)
            continue; // tinted exit markers
        tiles[i].type = TILE_INACTIVE;
    }
}

EMSCRIPTEN_KEEPALIVE void click(int x, int y) {
    // A switch takes a square click zone centred on its tile, wider than the
    // drawn diamond so it's easy to hit. There's always a belt or gap between
    // two switches, so the zones never overlap — first hit wins.
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type != TILE_SWITCH)
            continue;
        Pt p = project(tile_centre(i % GAME_W), tile_centre(i / GAME_W));
        if (abs(x - p.x) <= SWITCH_HIT && abs(y - p.y) <= SWITCH_HIT) {
            uint8_t mask = tiles[i].dirs_mask;
            for (int n = 1; n <= 4; n++) {
                uint8_t next = (tiles[i].dir + n) % 4;
                if (mask & (1 << next)) {
                    tiles[i].dir = next;
                    break;
                }
            }
            return;
        }
    }
}

#include "levels.inc"

/* ****************************** DRAWING ****************************** */

static const RGB COLOURS[COLOUR_COUNT] = {
    [COLOUR_RED] = {200, 50, 50},
    [COLOUR_BLUE] = {50, 50, 200},
    [COLOUR_GREEN] = {50, 200, 50},
    [COLOUR_YELLOW] = {200, 220, 0},
    [COLOUR_WHITE] = {150, 150, 150},
};

// filled diamond (TILE_W x TILE_H) centred at (cx,cy)... placeholder for sprites
static void fill_diamond(int cx, int cy, uint8_t r, uint8_t g, uint8_t b) {
    for (int dy = -HY; dy <= HY; dy++) {
        int half = HX - HX * abs(dy) / HY; // width tapers linearly to the tips
        for (int dx = -half; dx <= half; dx++)
            put_pixel(cx + dx, cy + dy, r, g, b);
    }
}

// 50% blue tint blend over the diamond footprint at (cx, cy)
static void blend_blue_diamond(int cx, int cy) {
    for (int dy = -HY; dy <= HY; dy++) {
        int half = HX - HX * abs(dy) / HY;
        for (int dx = -half; dx <= half; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
                continue;
            int i = PIXEL(x, y);
            fb[i] = fb[i] >> 1;
            fb[i + 1] = fb[i + 1] >> 1;
            fb[i + 2] = (uint8_t)((fb[i + 2] >> 1) + 128);
        }
    }
}

void draw_tile(Tile tile, int col, int row) {
    if (tile.type != TILE_CONVEYOR && tile.type != TILE_SWITCH && tile.type != TILE_EXIT) {
        return; // Remove guard when all the tiles are finished
    }
    Pt p = project(tile_centre(col), tile_centre(row));

    if (tile.type == TILE_EXIT) {
        // no exit art yet: live exit shows colour, exhausted goes black
        RGB c = tile.capacity > 0 ? COLOURS[tile.colour] : (RGB){0, 0, 0};
        fill_diamond(p.x, p.y, c.r, c.g, c.b);
        return;
    }

    // CONVEYOR / SWITCH: a directional sprite, centred on the diamond.
    int base = (tile.type == TILE_SWITCH) ? SPR_SWITCH_UP : SPR_TILE_UP;
    int id = base + tile.dir;
    blit_sprite(id, p.x - sprites[id].w / 2, p.y - sprites[id].h / 2);
    // capacity flag on a conveyor means: overlay a 50% blue tint (marks potential exits) (DEBUG)
    if (tile.type == TILE_CONVEYOR && tile.capacity)
        blend_blue_diamond(p.x, p.y);
}

void draw_background() {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            int temp = (x + y + S.tick) % 512;
            int i = PIXEL(x, y);

            fb[i] = temp < 256 ? temp : 511 - temp;
            temp = (temp + 170) % 512;
            fb[i + 1] = temp < 256 ? temp : 511 - temp;
            temp = (temp + 170) % 512;
            fb[i + 2] = temp < 256 ? temp : 511 - temp;
            fb[i + 3] = 55;
        }
    }

    // Non-switch tiles first.
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type == TILE_SWITCH)
            continue;
        draw_tile(tiles[i], i % GAME_W, i / GAME_W);
    }
    // Switches last: the circular sprite overhangs neighbouring tiles, so it
    // must paint after every adjacent tile is already down.
    for (int i = 0; i < NUM_TILES; i++) {
        if (tiles[i].type != TILE_SWITCH)
            continue;
        draw_tile(tiles[i], i % GAME_W, i / GAME_W);
    }
}

// blit an RGBA sprite at (dx,dy); skip fully transparent pixels (1-bit alpha)
static void blit(const uint8_t *src, int width, int height, int dx, int dy) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            const uint8_t *px = &src[(y * width + x) * 4];
            if (px[3] == 0)
                continue;
            put_pixel(dx + x, dy + y, px[0], px[1], px[2]);
        }
}

static void blit_sprite(int id, int dx, int dy) {
    Sprite *s = &sprites[id];
    blit(s->rgba, s->w, s->h, dx, dy);
}

void draw_boxes() {
    for (int i = 0; i < MAX_BOXES; i++) {
        if (!box[i].active) {
            continue;
        }
        Pt p = project(box[i].x, box[i].y);
        int id = SPR_BOX_RED + box[i].colour;
        // always anchor to bottom because boxes stick up over top of their tile
        blit_sprite(id, p.x - sprites[id].w / 2, p.y + HY - sprites[id].h);
    }
}

int get_grid(Box box) {
    return world_to_tile(box.x) + world_to_tile(box.y) * GAME_W;
}

// put_pixel, safe for out of range coordinates
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return;
    int i = PIXEL(x, y);
    fb[i + 0] = r;
    fb[i + 1] = g;
    fb[i + 2] = b;
    fb[i + 3] = 255;
}

// Bresenham's line algorithm — integer-only, handles every octant.
// Probably won't be required now I'm using sprites
void draw_line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put_pixel(x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            put_pixel(i, j, r, g, b);
}
