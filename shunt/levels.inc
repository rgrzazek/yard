/* ****************************** LEVELS  ****************************** */
// We have an array of function pointers for each level

static void set_tile(int x, int y, uint8_t type, uint8_t dir) {
    int i = x + y * GAME_W;
    tiles[i].type = type;
    tiles[i].dir = dir;
}

static void set_exit(int x, int y, uint8_t colour, uint8_t capacity) {
    int i = x + y * GAME_W;
    tiles[i].type = TILE_EXIT;
    tiles[i].colour = colour;
    tiles[i].capacity = capacity;
}

static void load_level_1(void) {
    // Standard reset: everything inactive.
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    for (int col = 2; col <= 16; col++) {
        set_tile(10, col, TILE_CONVEYOR, DOWN);
    }
    set_exit(10, 17, rand() % COLOUR_COUNT, 2);

    set_exit(2, 10, rand() % COLOUR_COUNT, 2);
    for (int row = 3; row <= 9; row++) {
        set_tile(row, 10, TILE_CONVEYOR, LEFT);
    }
    set_tile(10, 10, TILE_SWITCH, DOWN); // intersection

    for (int row = 11; row <= 16; row++) {
        set_tile(row, 10, TILE_CONVEYOR, RIGHT);
    }
    set_exit(17, 10, rand() % COLOUR_COUNT, 2);
}

// Add load_level_2, load_level_3, ... above, then list them here in order.
static void (*const levels[])(void) = {
    load_level_1,
};
#define LEVEL_COUNT ((int)(sizeof levels / sizeof levels[0]))

static void load_level(int n) {
    if (n < 1 || n > LEVEL_COUNT)
        return; // out of range: leave the board as-is
    levels[n - 1]();
}
