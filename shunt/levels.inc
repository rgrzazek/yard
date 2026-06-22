/* ****************************** LEVELS  ****************************** */
// We have an array of function pointers for each level

static void set_tile(int x, int y, uint8_t type, uint8_t dir) {
    int i = x + y * GAME_W;
    tiles[i].type = type;
    tiles[i].dir = dir;
}

// dirs_mask: bitmask of directions this switch may cycle through, bit = (1 << Dir).
// E.g. (1<<RIGHT)|(1<<DOWN)|(1<<LEFT) excludes UP.
static void set_switch(int x, int y, uint8_t dir, uint8_t dirs_mask) {
    int i = x + y * GAME_W;
    tiles[i].type = TILE_SWITCH;
    tiles[i].dir = dir;
    tiles[i].dirs_mask = dirs_mask;
}

static void set_exit(int x, int y, uint8_t colour, uint8_t capacity) {
    int i = x + y * GAME_W;
    tiles[i].type = TILE_EXIT;
    tiles[i].colour = colour;
    tiles[i].capacity = capacity;
}

// Conveyor with capacity flag set — triggers blue tint overlay in draw_tile
static void set_tile_tinted(int x, int y, uint8_t dir) {
    int i = x + y * GAME_W;
    tiles[i].type = TILE_CONVEYOR;
    tiles[i].dir = dir;
    tiles[i].capacity = 1;
}

static void load_level_1(void) {
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    set_tile(ORIGIN_COL, ORIGIN_ROW, TILE_CONVEYOR, DOWN);

    set_exit( 4,  4, rand() % COLOUR_COUNT, 1);
    set_tile_tinted( 2,  6, UP);
    set_tile_tinted( 0,  8, UP);
    set_exit( 4, 15, rand() % COLOUR_COUNT, 1);
    set_tile_tinted( 6, 17, LEFT);
    set_tile_tinted( 8, 19, LEFT);
    set_tile_tinted(15, 15, DOWN);
    set_exit(17, 13, rand() % COLOUR_COUNT, 1);
    set_tile_tinted(19, 11, DOWN);

    set_tile(10,  2, TILE_CONVEYOR, DOWN);
    set_tile(10,  3, TILE_CONVEYOR, DOWN);
    set_tile(10,  4, TILE_CONVEYOR, DOWN);
    set_tile( 4,  5, TILE_CONVEYOR, UP);
    set_tile(10,  5, TILE_CONVEYOR, DOWN);
    set_tile( 4,  6, TILE_CONVEYOR, UP);
    set_tile( 5,  6, TILE_CONVEYOR, LEFT);
    set_tile( 6,  6, TILE_CONVEYOR, LEFT);
    set_tile( 7,  6, TILE_CONVEYOR, LEFT);
    set_tile( 8,  6, TILE_CONVEYOR, LEFT);
    set_tile( 9,  6, TILE_CONVEYOR, LEFT);
    set_switch(10,  6, DOWN, (1<<LEFT)|(1<<DOWN));
    set_tile(11,  6, TILE_CONVEYOR, LEFT);
    set_tile(12,  6, TILE_CONVEYOR, LEFT);
    set_tile(13,  6, TILE_CONVEYOR, LEFT);
    set_tile(10,  7, TILE_CONVEYOR, DOWN);
    set_tile(13,  7, TILE_CONVEYOR, UP);
    set_tile(10,  8, TILE_CONVEYOR, DOWN);
    set_tile(13,  8, TILE_CONVEYOR, UP);
    set_tile(10,  9, TILE_CONVEYOR, DOWN);
    set_tile(13,  9, TILE_CONVEYOR, UP);
    set_tile(10, 10, TILE_CONVEYOR, DOWN);
    set_tile(13, 10, TILE_CONVEYOR, UP);
    set_tile(10, 11, TILE_CONVEYOR, DOWN);
    set_tile(13, 11, TILE_CONVEYOR, UP);
    set_tile(10, 12, TILE_CONVEYOR, DOWN);
    set_switch(13,  12, UP, (1<<UP)|(1<<RIGHT));
    set_tile(14, 12, TILE_CONVEYOR, RIGHT);
    set_tile(15, 12, TILE_CONVEYOR, RIGHT);
    set_tile(16, 12, TILE_CONVEYOR, RIGHT);
    set_tile(17, 12, TILE_CONVEYOR, DOWN);
    set_tile(10, 13, TILE_CONVEYOR, DOWN);
    set_tile(13, 13, TILE_CONVEYOR, UP);
    set_tile(10, 14, TILE_CONVEYOR, DOWN);
    set_tile(13, 14, TILE_CONVEYOR, UP);
    set_tile( 5, 15, TILE_CONVEYOR, LEFT);
    set_tile( 6, 15, TILE_CONVEYOR, LEFT);
    set_tile( 7, 15, TILE_CONVEYOR, LEFT);
    set_tile( 8, 15, TILE_CONVEYOR, LEFT);
    set_tile( 9, 15, TILE_CONVEYOR, LEFT);
    set_switch(10, 15, LEFT, (1<<LEFT)|(1<<RIGHT));
    set_tile(11, 15, TILE_CONVEYOR, RIGHT);
    set_tile(12, 15, TILE_CONVEYOR, RIGHT);
    set_tile(13, 15, TILE_CONVEYOR, UP);
}

static void load_level_2(void) {
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    set_tile(ORIGIN_COL, ORIGIN_ROW, TILE_CONVEYOR, DOWN);

    // Inactive potential exit markers
    set_tile_tinted( 2,  6, UP);
    set_tile_tinted( 6, 17, LEFT);
    set_tile_tinted(15, 15, DOWN);

    // Active exits
    set_exit( 4,  4, rand() % COLOUR_COUNT, 1);
    set_exit( 0,  8, rand() % COLOUR_COUNT, 1);
    set_exit( 4, 15, rand() % COLOUR_COUNT, 1);
    set_exit( 8, 19, rand() % COLOUR_COUNT, 1);
    set_exit(17, 13, rand() % COLOUR_COUNT, 1);
    set_exit(19, 11, rand() % COLOUR_COUNT, 1);

    // Entry spine: feeds into row 6 at (10,6)
    set_tile(10,  2, TILE_CONVEYOR, DOWN);
    set_tile(10,  3, TILE_CONVEYOR, DOWN);
    set_tile(10,  4, TILE_CONVEYOR, DOWN);
    set_tile(10,  5, TILE_CONVEYOR, DOWN);

    // Row 6 — LEFT; spine joins at (10,6) as a plain LEFT tile
    set_tile(13,  6, TILE_CONVEYOR, LEFT);
    set_tile(12,  6, TILE_CONVEYOR, LEFT);
    set_tile(11,  6, TILE_CONVEYOR, LEFT);
    set_tile(10,  6, TILE_CONVEYOR, LEFT);
    set_tile( 9,  6, TILE_CONVEYOR, LEFT);
    set_tile( 8,  6, TILE_CONVEYOR, LEFT);
    set_tile( 7,  6, TILE_CONVEYOR, LEFT);
    set_switch( 6,  6, DOWN, (1<<LEFT)|(1<<DOWN));   // exit (4,4) or enter col 6

    // Exit (4,4) branch
    set_tile( 5,  6, TILE_CONVEYOR, LEFT);
    set_tile( 4,  6, TILE_CONVEYOR, UP);
    set_tile( 4,  5, TILE_CONVEYOR, UP);

    // Col 6 — DOWN
    set_tile( 6,  7, TILE_CONVEYOR, DOWN);
    set_switch( 6,  8, DOWN, (1<<LEFT)|(1<<DOWN));   // exit (0,8) or continue
    set_tile( 6,  9, TILE_CONVEYOR, DOWN);
    set_tile( 6, 10, TILE_CONVEYOR, DOWN);
    set_tile( 6, 11, TILE_CONVEYOR, DOWN);
    set_tile( 6, 12, TILE_CONVEYOR, DOWN);
    set_tile( 6, 13, TILE_CONVEYOR, DOWN);
    set_tile( 6, 14, TILE_CONVEYOR, DOWN);
    set_switch( 6, 15, RIGHT, (1<<LEFT)|(1<<RIGHT)); // exit (4,15) or enter row 15

    // Exit (0,8) branch
    set_tile( 5,  8, TILE_CONVEYOR, LEFT);
    set_tile( 4,  8, TILE_CONVEYOR, LEFT);
    set_tile( 3,  8, TILE_CONVEYOR, LEFT);
    set_tile( 2,  8, TILE_CONVEYOR, LEFT);
    set_tile( 1,  8, TILE_CONVEYOR, LEFT);

    // Exit (4,15) branch
    set_tile( 5, 15, TILE_CONVEYOR, LEFT);

    // Row 15 — RIGHT
    set_tile( 7, 15, TILE_CONVEYOR, RIGHT);
    set_switch( 8, 15, RIGHT, (1<<RIGHT)|(1<<DOWN)); // exit (8,19) or continue
    set_tile( 9, 15, TILE_CONVEYOR, RIGHT);
    set_tile(10, 15, TILE_CONVEYOR, RIGHT);
    set_tile(11, 15, TILE_CONVEYOR, RIGHT);
    set_tile(12, 15, TILE_CONVEYOR, RIGHT);
    set_tile(13, 15, TILE_CONVEYOR, UP);             // corner: turn UP into col 13

    // Exit (8,19) branch
    set_tile( 8, 16, TILE_CONVEYOR, DOWN);
    set_tile( 8, 17, TILE_CONVEYOR, DOWN);
    set_tile( 8, 18, TILE_CONVEYOR, DOWN);

    // Col 13 — UP
    set_tile(13, 14, TILE_CONVEYOR, UP);
    set_switch(13, 13, UP, (1<<UP)|(1<<RIGHT));      // exit (17,13) or continue
    set_tile(13, 12, TILE_CONVEYOR, UP);
    set_switch(13, 11, UP, (1<<UP)|(1<<RIGHT));      // exit (19,11) or continue
    set_tile(13, 10, TILE_CONVEYOR, UP);
    set_tile(13,  9, TILE_CONVEYOR, UP);
    set_tile(13,  8, TILE_CONVEYOR, UP);
    set_tile(13,  7, TILE_CONVEYOR, UP);

    // Exit (17,13) branch
    set_tile(14, 13, TILE_CONVEYOR, RIGHT);
    set_tile(15, 13, TILE_CONVEYOR, RIGHT);
    set_tile(16, 13, TILE_CONVEYOR, RIGHT);

    // Exit (19,11) branch
    set_tile(14, 11, TILE_CONVEYOR, RIGHT);
    set_tile(15, 11, TILE_CONVEYOR, RIGHT);
    set_tile(16, 11, TILE_CONVEYOR, RIGHT);
    set_tile(17, 11, TILE_CONVEYOR, RIGHT);
    set_tile(18, 11, TILE_CONVEYOR, RIGHT);
}

static void load_level_3(void) {
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    set_tile(ORIGIN_COL, ORIGIN_ROW, TILE_CONVEYOR, DOWN);

        // Active exits
    set_exit( 4,  4, rand() % COLOUR_COUNT, 1);
    set_exit( 2,  6, rand() % COLOUR_COUNT, 1);
    set_exit( 0,  8, rand() % COLOUR_COUNT, 1);
    set_exit( 4, 15, rand() % COLOUR_COUNT, 1);
    set_exit( 6, 17, rand() % COLOUR_COUNT, 1);
    set_exit( 8, 19, rand() % COLOUR_COUNT, 1);
    set_exit(15, 15, rand() % COLOUR_COUNT, 1);
    set_exit(17, 13, rand() % COLOUR_COUNT, 1);
    set_exit(19, 11, rand() % COLOUR_COUNT, 1);

    set_tile(10,  2, TILE_CONVEYOR, DOWN);
    set_tile(10,  3, TILE_CONVEYOR, DOWN);
    set_tile(10,  4, TILE_CONVEYOR, DOWN);
    set_tile( 4,  5, TILE_CONVEYOR, UP);
    set_tile(10,  5, TILE_CONVEYOR, DOWN);
    set_tile( 4,  6, TILE_CONVEYOR, UP);
    set_tile(10,  6, TILE_CONVEYOR, DOWN);
    set_tile( 2,  7, TILE_CONVEYOR, UP);
    set_tile( 4,  7, TILE_CONVEYOR, UP);
    set_tile(10,  7, TILE_CONVEYOR, DOWN);
    set_tile( 2,  8, TILE_CONVEYOR, UP);
    set_tile( 4,  8, TILE_CONVEYOR, UP);
    set_tile(10,  8, TILE_CONVEYOR, DOWN);
    set_tile(13,  8, TILE_CONVEYOR, RIGHT);
    set_tile(14,  8, TILE_CONVEYOR, RIGHT);
    set_tile(15,  8, TILE_CONVEYOR, RIGHT);
    set_tile(16,  8, TILE_CONVEYOR, RIGHT);
    set_tile(17,  8, TILE_CONVEYOR, RIGHT);
    set_tile(18,  8, TILE_CONVEYOR, RIGHT);
    set_tile(19,  8, TILE_CONVEYOR, DOWN);
    set_tile( 0,  9, TILE_CONVEYOR, UP);
    set_tile( 2,  9, TILE_CONVEYOR, UP);
    set_tile( 3,  9, TILE_CONVEYOR, LEFT);
    set_switch( 4,  9, DOWN, (1<<UP)|(1<<DOWN)|(1<<LEFT));
    set_tile( 5,  9, TILE_CONVEYOR, LEFT);
    set_tile( 6,  9, TILE_CONVEYOR, LEFT);
    set_tile( 7,  9, TILE_CONVEYOR, LEFT);
    set_tile( 8,  9, TILE_CONVEYOR, LEFT);
    set_tile( 9,  9, TILE_CONVEYOR, LEFT);
    set_switch(10,  9, DOWN, (1<<LEFT)|(1<<DOWN));
    set_tile(13,  9, TILE_CONVEYOR, UP);
    set_tile(19,  9, TILE_CONVEYOR, DOWN);
    set_tile( 0, 10, TILE_CONVEYOR, UP);
    set_tile( 4, 10, TILE_CONVEYOR, DOWN);
    set_tile(10, 10, TILE_CONVEYOR, DOWN);
    set_switch(13, 10, UP, (1<<RIGHT)|(1<<UP));
    set_tile(14, 10, TILE_CONVEYOR, RIGHT);
    set_switch(15, 10, DOWN, (1<<RIGHT)|(1<<DOWN));
    set_tile(16, 10, TILE_CONVEYOR, RIGHT);
    set_tile(17, 10, TILE_CONVEYOR, DOWN);
    set_tile(19, 10, TILE_CONVEYOR, DOWN);
    set_tile( 0, 11, TILE_CONVEYOR, UP);
    set_tile( 1, 11, TILE_CONVEYOR, LEFT);
    set_tile( 2, 11, TILE_CONVEYOR, LEFT);
    set_tile( 3, 11, TILE_CONVEYOR, LEFT);
    set_tile( 4, 11, TILE_CONVEYOR, LEFT);
    set_tile(10, 11, TILE_CONVEYOR, DOWN);
    set_tile(13, 11, TILE_CONVEYOR, UP);
    set_tile(15, 11, TILE_CONVEYOR, DOWN);
    set_tile(17, 11, TILE_CONVEYOR, DOWN);
    set_tile(10, 12, TILE_CONVEYOR, DOWN);
    set_tile(13, 12, TILE_CONVEYOR, UP);
    set_tile(15, 12, TILE_CONVEYOR, DOWN);
    set_tile(17, 12, TILE_CONVEYOR, DOWN);
    set_tile(10, 13, TILE_CONVEYOR, DOWN);
    set_tile(13, 13, TILE_CONVEYOR, UP);
    set_tile(15, 13, TILE_CONVEYOR, DOWN);
    set_tile(10, 14, TILE_CONVEYOR, DOWN);
    set_tile(13, 14, TILE_CONVEYOR, UP);
    set_tile(15, 14, TILE_CONVEYOR, DOWN);
    set_tile( 5, 15, TILE_CONVEYOR, LEFT);
    set_tile( 6, 15, TILE_CONVEYOR, LEFT);
    set_tile( 7, 15, TILE_CONVEYOR, LEFT);
    set_tile( 8, 15, TILE_CONVEYOR, LEFT);
    set_tile( 9, 15, TILE_CONVEYOR, LEFT);
    set_switch(10, 15, DOWN, (1<<RIGHT)|(1<<DOWN)|(1<<LEFT));
    set_tile(11, 15, TILE_CONVEYOR, RIGHT);
    set_tile(12, 15, TILE_CONVEYOR, RIGHT);
    set_tile(13, 15, TILE_CONVEYOR, UP);
    set_tile(10, 16, TILE_CONVEYOR, DOWN);
    set_tile( 7, 17, TILE_CONVEYOR, LEFT);
    set_tile( 8, 17, TILE_CONVEYOR, LEFT);
    set_tile( 9, 17, TILE_CONVEYOR, LEFT);
    set_switch(10, 17, DOWN, (1<<LEFT)|(1<<DOWN));
    set_tile(10, 18, TILE_CONVEYOR, DOWN);
    set_tile( 9, 19, TILE_CONVEYOR, LEFT);
    set_tile(10, 19, TILE_CONVEYOR, LEFT);
}

static void load_level_4(void) {
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    set_tile(ORIGIN_COL, ORIGIN_ROW, TILE_CONVEYOR, DOWN);

        // Active exits
    set_exit( 4,  4, rand() % COLOUR_COUNT, 1);
    set_exit( 2,  6, rand() % COLOUR_COUNT, 1);
    set_exit( 0,  8, rand() % COLOUR_COUNT, 1);
    set_exit( 4, 15, rand() % COLOUR_COUNT, 1);
    set_exit( 6, 17, rand() % COLOUR_COUNT, 1);
    set_exit( 8, 19, rand() % COLOUR_COUNT, 1);
    set_exit(15, 15, rand() % COLOUR_COUNT, 1);
    set_exit(17, 13, rand() % COLOUR_COUNT, 1);
    set_exit(19, 11, rand() % COLOUR_COUNT, 1);

    set_tile(10,  2, TILE_CONVEYOR, DOWN);
    set_tile(10,  3, TILE_CONVEYOR, DOWN);
    set_tile(10,  4, TILE_CONVEYOR, DOWN);
    set_tile( 4,  5, TILE_CONVEYOR, UP);
    set_switch( 10,  5, DOWN, (1<<RIGHT)|(1<<DOWN));
    set_tile(11,  5, TILE_CONVEYOR, RIGHT);
    set_tile(12,  5, TILE_CONVEYOR, RIGHT);
    set_tile(13,  5, TILE_CONVEYOR, RIGHT);
    set_tile(14,  5, TILE_CONVEYOR, RIGHT);
    set_tile(15,  5, TILE_CONVEYOR, RIGHT);
    set_tile(16,  5, TILE_CONVEYOR, RIGHT);
    set_tile(17,  5, TILE_CONVEYOR, RIGHT);
    set_tile(18,  5, TILE_CONVEYOR, RIGHT);
    set_tile(19,  5, TILE_CONVEYOR, DOWN);
    set_tile( 4,  6, TILE_CONVEYOR, UP);
    set_tile(10,  6, TILE_CONVEYOR, DOWN);
    set_tile(19,  6, TILE_CONVEYOR, DOWN);
    set_tile( 2,  7, TILE_CONVEYOR, UP);
    set_tile( 4,  7, TILE_CONVEYOR, UP);
    set_tile(10,  7, TILE_CONVEYOR, DOWN);
    set_tile(19,  7, TILE_CONVEYOR, DOWN);
    set_tile( 2,  8, TILE_CONVEYOR, UP);
    set_tile( 3,  8, TILE_CONVEYOR, LEFT);
    set_switch( 4,  8, DOWN, (1<<UP)|(1<<DOWN)|(1<<LEFT));
    set_tile( 5,  8, TILE_CONVEYOR, LEFT);
    set_tile( 6,  8, TILE_CONVEYOR, LEFT);
    set_tile( 7,  8, TILE_CONVEYOR, LEFT);
    set_tile( 8,  8, TILE_CONVEYOR, LEFT);
    set_tile( 9,  8, TILE_CONVEYOR, LEFT);
    set_switch( 10,  8, DOWN, (1<<DOWN)|(1<<LEFT));    set_tile(19,  8, TILE_CONVEYOR, DOWN);
    set_tile( 0,  9, TILE_CONVEYOR, UP);
    set_tile( 4,  9, TILE_CONVEYOR, DOWN);
    set_tile(10,  9, TILE_CONVEYOR, DOWN);
    set_tile(11,  9, TILE_CONVEYOR, LEFT);
    set_tile(12,  9, TILE_CONVEYOR, LEFT);
    set_tile(13,  9, TILE_CONVEYOR, LEFT);
    set_tile(14,  9, TILE_CONVEYOR, LEFT);
    set_switch( 15,  9, DOWN, (1<<DOWN)|(1<<LEFT));
    set_tile(16,  9, TILE_CONVEYOR, LEFT);
    set_switch( 17,  9, DOWN, (1<<DOWN)|(1<<LEFT));
    set_tile(18,  9, TILE_CONVEYOR, LEFT);
    set_switch( 19,  9, DOWN, (1<<DOWN)|(1<<LEFT));
    set_tile( 0, 10, TILE_CONVEYOR, UP);
    set_tile( 1, 10, TILE_CONVEYOR, LEFT);
    set_tile( 2, 10, TILE_CONVEYOR, LEFT);
    set_tile( 3, 10, TILE_CONVEYOR, LEFT);
    set_tile( 4, 10, TILE_CONVEYOR, LEFT);
    set_tile(10, 10, TILE_CONVEYOR, DOWN);
    set_tile(15, 10, TILE_CONVEYOR, DOWN);
    set_tile(17, 10, TILE_CONVEYOR, DOWN);
    set_tile(19, 10, TILE_CONVEYOR, DOWN);
    set_tile(10, 11, TILE_CONVEYOR, DOWN);
    set_tile(15, 11, TILE_CONVEYOR, DOWN);
    set_tile(17, 11, TILE_CONVEYOR, DOWN);
    set_tile( 7, 12, TILE_CONVEYOR, DOWN);
    set_tile( 8, 12, TILE_CONVEYOR, LEFT);
    set_tile( 9, 12, TILE_CONVEYOR, LEFT);
    set_switch( 10,  12, DOWN, (1<<DOWN)|(1<<LEFT));
    set_tile(15, 12, TILE_CONVEYOR, DOWN);
    set_tile(17, 12, TILE_CONVEYOR, DOWN);
    set_tile( 7, 13, TILE_CONVEYOR, DOWN);
    set_tile(10, 13, TILE_CONVEYOR, DOWN);
    set_tile(15, 13, TILE_CONVEYOR, DOWN);
    set_tile( 7, 14, TILE_CONVEYOR, DOWN);
    set_tile(10, 14, TILE_CONVEYOR, DOWN);
    set_tile(15, 14, TILE_CONVEYOR, DOWN);
    set_tile( 5, 15, TILE_CONVEYOR, LEFT);
    set_tile( 6, 15, TILE_CONVEYOR, LEFT);
    set_switch( 7,  15, DOWN, (1<<DOWN)|(1<<LEFT)|(1<<RIGHT));
    set_tile( 8, 15, TILE_CONVEYOR, RIGHT);
    set_tile( 9, 15, TILE_CONVEYOR, DOWN);
    set_tile(10, 15, TILE_CONVEYOR, LEFT);
    set_tile( 7, 16, TILE_CONVEYOR, DOWN);
    set_tile( 9, 16, TILE_CONVEYOR, DOWN);
    set_tile( 7, 17, TILE_CONVEYOR, LEFT);
    set_tile( 9, 17, TILE_CONVEYOR, DOWN);
    set_tile( 9, 18, TILE_CONVEYOR, DOWN);
    set_tile( 9, 19, TILE_CONVEYOR, LEFT);
}

static void load_level_5(void) {
    for (int i = 0; i < NUM_TILES; i++) {
        tiles[i].type = TILE_INACTIVE;
    }

    set_tile(ORIGIN_COL, ORIGIN_ROW, TILE_CONVEYOR, DOWN);

    set_exit( 4,  4, rand() % COLOUR_COUNT, 1);
    set_exit( 2,  6, rand() % COLOUR_COUNT, 1);
    set_exit( 0,  8, rand() % COLOUR_COUNT, 1);
    set_exit( 4, 15, rand() % COLOUR_COUNT, 1);
    set_exit( 6, 17, rand() % COLOUR_COUNT, 1);
    set_exit( 8, 19, rand() % COLOUR_COUNT, 1);
    set_exit(15, 15, rand() % COLOUR_COUNT, 1);
    set_exit(17, 13, rand() % COLOUR_COUNT, 1);
    set_exit(19, 11, rand() % COLOUR_COUNT, 1);

    set_tile(10,  2, TILE_CONVEYOR, DOWN);
    set_tile( 6,  3, TILE_CONVEYOR, DOWN);
    set_tile( 7,  3, TILE_CONVEYOR, LEFT);
    set_tile( 8,  3, TILE_CONVEYOR, LEFT);
    set_tile( 9,  3, TILE_CONVEYOR, LEFT);
    set_switch(10,  3, DOWN, (1<<DOWN)|(1<<LEFT));
    set_tile( 6,  4, TILE_CONVEYOR, DOWN);
    set_tile(10,  4, TILE_CONVEYOR, DOWN);
    set_tile( 4,  5, TILE_CONVEYOR, UP);
    set_tile( 6,  5, TILE_CONVEYOR, DOWN);
    set_tile( 8,  5, TILE_CONVEYOR, DOWN);
    set_tile( 9,  5, TILE_CONVEYOR, LEFT);
    set_switch(10,  5, RIGHT, (1<<RIGHT)|(1<<LEFT));
    set_tile(11,  5, TILE_CONVEYOR, RIGHT);
    set_tile(12,  5, TILE_CONVEYOR, RIGHT);
    set_tile(13,  5, TILE_CONVEYOR, RIGHT);
    set_tile(14,  5, TILE_CONVEYOR, DOWN);
    set_tile( 4,  6, TILE_CONVEYOR, UP);
    set_tile( 6,  6, TILE_CONVEYOR, DOWN);
    set_tile( 8,  6, TILE_CONVEYOR, DOWN);
    set_tile(14,  6, TILE_CONVEYOR, DOWN);
    set_tile( 2,  7, TILE_CONVEYOR, UP);
    set_tile( 4,  7, TILE_CONVEYOR, UP);
    set_tile( 6,  7, TILE_CONVEYOR, DOWN);
    set_tile( 8,  7, TILE_CONVEYOR, RIGHT);
    set_tile( 9,  7, TILE_CONVEYOR, RIGHT);
    set_tile(10,  7, TILE_CONVEYOR, RIGHT);
    set_tile(11,  7, TILE_CONVEYOR, RIGHT);
    set_tile(12,  7, TILE_CONVEYOR, DOWN);
    set_tile(14,  7, TILE_CONVEYOR, DOWN);
    set_tile( 2,  8, TILE_CONVEYOR, UP);
    set_tile( 4,  8, TILE_CONVEYOR, UP);
    set_tile( 6,  8, TILE_CONVEYOR, DOWN);
    set_tile(12,  8, TILE_CONVEYOR, DOWN);
    set_tile(14,  8, TILE_CONVEYOR, DOWN);
    set_tile( 0,  9, TILE_CONVEYOR, UP);
    set_tile( 2,  9, TILE_CONVEYOR, UP);
    set_tile( 4,  9, TILE_CONVEYOR, UP);
    set_tile( 6,  9, TILE_CONVEYOR, DOWN);
    set_tile(12,  9, TILE_CONVEYOR, DOWN);
    set_tile(14,  9, TILE_CONVEYOR, RIGHT);
    set_switch(15,  9, RIGHT, (1<<RIGHT)|(1<<DOWN));
    set_tile(16,  9, TILE_CONVEYOR, RIGHT);
    set_switch(17,  9, DOWN, (1<<RIGHT)|(1<<DOWN));
    set_tile(18,  9, TILE_CONVEYOR, RIGHT);
    set_tile(19,  9, TILE_CONVEYOR, DOWN);
    set_tile( 0, 10, TILE_CONVEYOR, UP);
    set_tile( 1, 10, TILE_CONVEYOR, LEFT);
    set_switch( 2, 10, UP, (1<<UP)|(1<<LEFT)|(1<<RIGHT));
    set_tile( 3, 10, TILE_CONVEYOR, RIGHT);
    set_tile( 4, 10, TILE_CONVEYOR, UP);
    set_tile( 6, 10, TILE_CONVEYOR, DOWN);
    set_tile(12, 10, TILE_CONVEYOR, DOWN);
    set_tile(15, 10, TILE_CONVEYOR, DOWN);
    set_tile(17, 10, TILE_CONVEYOR, DOWN);
    set_tile(19, 10, TILE_CONVEYOR, DOWN);
    set_tile( 2, 11, TILE_CONVEYOR, UP);
    set_tile( 6, 11, TILE_CONVEYOR, DOWN);
    set_tile(12, 11, TILE_CONVEYOR, DOWN);
    set_tile(15, 11, TILE_CONVEYOR, DOWN);
    set_tile(17, 11, TILE_CONVEYOR, DOWN);
    set_tile( 2, 12, TILE_CONVEYOR, UP);
    set_tile( 6, 12, TILE_CONVEYOR, DOWN);
    set_tile(12, 12, TILE_CONVEYOR, DOWN);
    set_tile(15, 12, TILE_CONVEYOR, DOWN);
    set_tile(17, 12, TILE_CONVEYOR, DOWN);
    set_tile( 2, 13, TILE_CONVEYOR, UP);
    set_tile( 3, 13, TILE_CONVEYOR, LEFT);
    set_tile( 4, 13, TILE_CONVEYOR, LEFT);
    set_tile( 5, 13, TILE_CONVEYOR, LEFT);
    set_tile( 6, 13, TILE_CONVEYOR, LEFT);
    set_tile(12, 13, TILE_CONVEYOR, DOWN);
    set_tile(15, 13, TILE_CONVEYOR, DOWN);
    set_tile(12, 14, TILE_CONVEYOR, DOWN);
    set_tile(15, 14, TILE_CONVEYOR, DOWN);
    set_tile( 5, 15, TILE_CONVEYOR, LEFT);
    set_tile( 6, 15, TILE_CONVEYOR, LEFT);
    set_tile( 7, 15, TILE_CONVEYOR, LEFT);
    set_tile( 8, 15, TILE_CONVEYOR, LEFT);
    set_tile( 9, 15, TILE_CONVEYOR, LEFT);
    set_tile(10, 15, TILE_CONVEYOR, LEFT);
    set_tile(12, 15, TILE_CONVEYOR, DOWN);
    set_tile(10, 16, TILE_CONVEYOR, UP);
    set_tile(12, 16, TILE_CONVEYOR, DOWN);
    set_tile( 7, 17, TILE_CONVEYOR, LEFT);
    set_tile( 8, 17, TILE_CONVEYOR, LEFT);
    set_tile( 9, 17, TILE_CONVEYOR, LEFT);
    set_switch(10, 17, LEFT, (1<<UP)|(1<<LEFT));
    set_tile(12, 17, TILE_CONVEYOR, DOWN);
    set_tile(10, 18, TILE_CONVEYOR, UP);
    set_tile(12, 18, TILE_CONVEYOR, DOWN);
    set_tile( 9, 19, TILE_CONVEYOR, LEFT);
    set_switch(10, 19, LEFT, (1<<LEFT)|(1<<UP));
    set_tile(11, 19, TILE_CONVEYOR, LEFT);
    set_tile(12, 19, TILE_CONVEYOR, LEFT);
}

// Add load_level_6, ... above, then list them here in order.
static void (*const levels[])(void) = {
    load_level_1,
    load_level_2,
    load_level_3,
    load_level_4,
    load_level_5,
};
#define LEVEL_COUNT ((int)(sizeof levels / sizeof levels[0]))

static void load_level(int n) {
    if (n < 1 || n > LEVEL_COUNT)
        return; // out of range: leave the board as-is
    levels[n - 1]();
}
