#include "viewer.h"

void viewerFree(ViewerState *V) {
  // nothing to free yet
}

void viewerPrintDebug(ViewerState *V, struct abuf *ab) {
  char s[512];
  int len, l, w, h;
  float d;

  // ViewerState
  len = snprintf(s, sizeof(s),
                 "Viewer: \r\n"
                 "  cols, rows: %6d, %6d \r\n"
                 "    vw,   vh: %6d, %6d \r\n"
                 "    cw,   ch: %6d, %6d \r\n"
                 "    ww,   wh: %6d, %6d \r\n"
                 "    mx,   my: %6d, %6d \r\n\n"
                 "     l, x, y: %6d, %6d, %6d \r\n\n",
                 V->cols, V->rows, V->vw, V->vh, V->cw, V->ch, V->ww, V->wh,
                 V->mx, V->my, V->l, V->x, V->y);
  abAppend(ab, s, len);

  // SlideState
  l = V->S->level_count;
  len = snprintf(s, sizeof(s),
                 "Slide: \r\n"
                 "  level_count: %d\r\n"
                 "  Levels: \r\n",
                 l);
  abAppend(ab, s, len);

  // No choice but to iterate and grow ab
  for (int i = 0; i < l; i++) {
    w = V->S->level_w[i];
    h = V->S->level_h[i];
    d = V->S->downsamples[i];
    len = snprintf(s, sizeof(s), "    %2d: %6d, %6d, %f  \r\n", i, w, h, d);
    abAppend(ab, s, len);
  }

  // Thumbnail
  len = snprintf(s, sizeof(s),
                 "  Thumbail: \r\n"
                 "    w, h: %6ld, %6ld\r\n",
                 V->S->thumbnail_w, V->S->thumbnail_h);
  abAppend(ab, s, len);
}

void viewerInit(ViewerState *V) {
  // Clear screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // Read window size
  if (getWindowSize(&V->rows, &V->cols, &V->vw, &V->vh) == -1)
    die("getWindowSize");

  // Compute cell size
  V->cw = (int)V->vw / V->cols;
  V->ch = (int)V->vh / V->cols;

  // Store current l, x, y, world, view
  V->l = V->S->level_count - 1; // Lowest zoom
  V->x = 0;                     // Starting tile
  V->y = 0;                     //  at top left corner

  // World size is size of slide at level `l`
  V->ww = V->S->level_w[V->l];
  V->wh = V->S->level_h[V->l];

  // Set maximums for level, x, y
  V->ts = TILE_SIZE;
  V->ml = V->S->level_count;
  V->mx = (int)floor((float)V->vw / V->ts);
  V->my = (int)floor((float)V->vh / V->ts);

  // Send thumbnail to kitty
  int w = V->S->thumbnail_w;
  int h = V->S->thumbnail_h;
  // uint32_t *buf = malloc(w * h * sizeof(uint32_t));
  uint32_t *buf = V->S->thumbnail;
  provisionImage(THUMBNAIL_ID, w, h, buf);

  // Dirty for first render
  V->dirty = 1;
}

void viewerRender(ViewerState *V, struct abuf *ab) {
  displayImage(THUMBNAIL_ID, 0, 0, 0, 0, -1);
}

void viewerRefreshScreen(ViewerState *V) {
  if (!V->dirty)
    return;

  // Initialize a buffer
  // realloc each time we append??
  // why not decent initial size and double like arrays??
  struct abuf ab = ABUF_INIT;

  // clear screen, move to origin, hide cursor
  abAppend(&ab, "\x1b[J", 3);
  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25l", 6);

  // ... main render function ...
  viewerRender(V, &ab);

  // Print debug info
  viewerPrintDebug(V, &ab);

  // Finally write out
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

  // Reset dirty state
  V->dirty = 0;
}

void viewerHandleKeypress(ViewerState *V, int key) {
  switch (key) {
  case 'h':
  case ARROW_LEFT:
    V->x = MAX(V->x - 1, 0);
    V->dirty = 1;
    break;
  case 'l':
  case ARROW_RIGHT:
    V->x = MIN(V->x + 1, V->mx - 1);
    V->dirty = 1;
    break;
  case 'k':
  case ARROW_UP:
    V->y = MAX(V->y - 1, 0);
    V->dirty = 1;
    break;
  case 'j':
  case ARROW_DOWN:
    V->y = MIN(V->y + 1, V->my - 1);
    V->dirty = 1;
    break;
  case 'i':
    V->l = MAX(V->l - 1, 0);
    V->dirty = 1;
    break;
  case 'o':
    V->l = MIN(V->l + 1, V->ml - 1);
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
  case 'j':
  case 'k':
  case 'h':
  case 'l':
  case 'i':
  case 'o':
    viewerHandleKeypress(V, c);
    break;
  }
}
