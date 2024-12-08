#include <stdbool.h>
#include "pd_api.h"

static int CELL_SIZE = 8;
static int CELL_COLS; // set at runtime
static int CELL_ROWS;

static PlaydateAPI* pd = NULL;

static bool** fg = NULL;
static bool** bg = NULL;

static bool isPaused = false;

static bool isAlive(int x, int y) {
    return fg[(x + CELL_COLS) % CELL_COLS][(y + CELL_ROWS) % CELL_ROWS];
}

// Any live cell with fewer than 2 live neighbors dies, as if caused by underpopulation.
// Any live cell with more than 3 live neighbors dies, as if by overcrowding.
// Any live cell with two or 3 live neighbors lives on to the next generation.
// Any dead cell with exactly 3 live neighbors becomes a live cell.
static bool nextState(int x, int y) {
    int neighbors = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            if (isAlive(x + dx, y + dy)) neighbors++;
        }
    }

    if (isAlive(x, y)) {
        return neighbors == 2 || neighbors == 3;
    } else {
        return neighbors == 3;
    }
    return false;
}

static void step(void) {
    for (int y = 0; y < CELL_ROWS; y++) {
        for (int x = 0; x < CELL_COLS; x++) {
            bg[x][y] = nextState(x, y);
        }
    }

    bool** temp = fg;
    fg = bg;
    bg = temp;
}

static void randomizeGrid(void) {
    for (int y = 0; y < CELL_ROWS; y++) {
        for (int x = 0; x < CELL_COLS; x++) {
            fg[x][y] = (rand() % 2 == 0); // TODO make variable via crank
        }
    }
}

static void initGrid(void) {
    CELL_COLS = LCD_COLUMNS / CELL_SIZE;
    CELL_ROWS = LCD_ROWS / CELL_SIZE;
    fg = (bool**)malloc(sizeof(bool*) * CELL_COLS);
    bg = (bool**)malloc(sizeof(bool*) * CELL_COLS);
    for (int i = 0; i < CELL_COLS; i++) {
        fg[i] = (bool*)malloc(sizeof(bool) * CELL_ROWS);
        bg[i] = (bool*)malloc(sizeof(bool) * CELL_ROWS);
    }
    randomizeGrid();
}

static void freeGrid(void) {
    for (int i = 0; i < CELL_COLS; i++) {
        free(bg[i]);
        free(fg[i]);
    }
    free(bg);
    free(fg);
    bg = NULL;
    fg = NULL;
}

static void resizeCell(int newCellSize) {
    freeGrid();
    CELL_SIZE = newCellSize;
    initGrid();
}

static void drawGrid(void) {
    pd->graphics->clear(kColorWhite);
    for (int y = 0; y < CELL_ROWS; y++) {
        for (int x = 0; x < CELL_COLS; x++) {
            if (fg[x][y]) {
                int px = x * CELL_SIZE;
                int py = y * CELL_SIZE;
                pd->graphics->fillRect(px, py, CELL_SIZE, CELL_SIZE, kColorBlack);
            }
        }
    }
}

// to confirm, was having strange behavior when drawing outside of main update function. not sure if timing issue/etc.
static int update(void* ud) {
    PDButtons current, pushed, released;
    pd->system->getButtonState(&current, &pushed, &released);

    if (pushed & kButtonA) {
        randomizeGrid();
    }
    if (pushed & kButtonB) {
        isPaused = !isPaused;
    }
    if (pushed & kButtonUp) {
        resizeCell(CELL_SIZE + 1);
    }
    if (pushed & kButtonDown && CELL_SIZE > 1) {
        resizeCell(CELL_SIZE - 1);
    }
    if (!isPaused) {
        step();
    }
    drawGrid();
    pd->system->drawFPS(0, 0);

    return 1;
}

int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
    if (event == kEventInit) {
        pd = playdate;
        initGrid();
        pd->display->setRefreshRate(8);
        pd->system->setUpdateCallback(update, NULL);
    }
    return 0;
}