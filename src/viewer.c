#include "viewer.h"

void viewerFree(ViewerState *V) {
  // nothing to free yet
}

void viewerInit(ViewerState *V) {
  // Clear screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // Read window size
  if (getWindowSize(&V->rows, &V->cols, &V->vw, &V->vh) == -1)
    die("getWindowSize");

  // Dirty for first render
  V->dirty = 1;
}

void viewerRefreshScreen(ViewerState *V) {
  if (V->dirty == 0)
    return;

  // Initialize a buffer
  // realloc each time we append??
  // why not decent initial size and double like arrays??
  struct abuf ab = ABUF_INIT;

  // hide cursor
  abAppend(&ab, "\x1b[?25l", 6);

  // ... main render function ...

  // Finally write out
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

  // Reset dirty state
  V->dirty = 0;
}

void viewerHandleKeypress(ViewerState *V, int key) {
  switch (key) {
  case ARROW_LEFT:
  case ARROW_RIGHT:
  case ARROW_UP:
  case ARROW_DOWN:
    V->dirty = 1;
    break;
  }
}

void viewerProcessKeypress(ViewerState *V) {
  int c = getKeypress();
  switch (c) {
  // Quit
  case 'q':
  case (CTRL_KEY('q')):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  // Cursor movement
  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    viewerHandleKeypress(V, c);
    break;
  }
}
