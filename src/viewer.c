#include "viewer.h"
#include <stdio.h>
#include <stdlib.h>

void viewerFree(ViewerState *V) {
  bufferFree(V->B);
  slideFree(V->S);
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
                 "     l, x, y: %6d, %6d, %6d \r\n"
                 "    ol,ox,oy: %6d, %6d, %6d \r\n\n",
                 V->cols, V->rows, V->vw, V->vh, V->cw, V->ch, V->ww, V->wh,
                 V->mx, V->my, V->l, V->x, V->y, V->ol, V->ox, V->oy);
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

void slice(const char *str, char *result, size_t start, size_t end) {
  strncpy(result, str + start, end - start);
}

void viewerInit(ViewerState *V, char *slide) {
  // Clear screen
  clearScreen();
  moveCursor(0, 0);

  // Open slide, read slide props like level, dims
  slideInit(V->S, slide);

  // Read window size using IOCTL -> over ssh, vw, vh fail
  if (getWindowSize(&V->rows, &V->cols, &V->vw, &V->vh) == -1)
    die("getWindowSize");

  // Read window size using kitty
  {
    if (write(STDOUT_FILENO, "\x1b[14t", 5) < 0)
      die("Query window pixel dims\n");

    // Read response
    char str[16];
    int ch, n = 0;
    int p1 = -1;
    int p2 = -1;
    while (1) {
      ch = getKeypress();
      str[n] = ch;
      if (ch == 't') {
        break;
      }
      if (ch == ';') {
        p1 == -1 ? (p1 = n) : (p2 = n);
      }
      n++;
    }

    // Parse height and width
    char str_w[10];
    char str_h[10];
    slice(str, str_h, p1 + 1, p2);
    slice(str, str_w, p2 + 1, n);
    V->vw = atoi(str_w);
    V->vh = atoi(str_h);
  }

  // Compute cell size
  V->cw = (int)V->vw / V->cols;
  V->ch = (int)V->vh / V->rows;

  // Store current l, x, y, world, view
  V->l = V->S->level_count - 1; // Lowest zoom
  // V->l = 0; // Highest zoom
  V->x = 0; // Starting tile
  V->y = 0; //  at top left corner

  V->ol = 0; // Lowest zoom
  V->ox = 0; // Starting tile
  V->oy = 0; //  at top left corner

  // World size is size of slide at level `l`
  V->ww = V->S->level_w[V->l];
  V->wh = V->S->level_h[V->l];

  // Set maximums for level, x, y
  V->ts = TILE_SIZE;
  V->ml = V->S->level_count;
  V->mx = (int)floor((float)V->ww / V->ts);
  V->my = (int)floor((float)V->wh / V->ts);

  // Send data of thumbnail to kitty, show it
  int w = V->S->thumbnail_w;
  int h = V->S->thumbnail_h;
  uint32_t *buf = V->S->thumbnail;

  // Write thumbnail to kitty
  int total_size = w * h * sizeof(uint32_t);
  size_t base64_size = ((total_size + 2) / 3) * 4;
  uint8_t *buf64 = (uint8_t *)malloc(base64_size + 1);
  bufferProvisionImage(THUMBNAIL_ID, w, h, buf, buf64);
  free(buf64);

  // Display it
  bufferDisplayImage(THUMBNAIL_ID, 0, 0, 0, 0, 2);

  // Compute maximum tiles visible given tile size and term dims
  int vtx = (int)floor((float)V->vw / V->ts);
  int vty = (int)floor((float)V->vh / V->ts);

  // Initialize buffers
  bufferInit(V->B, vtx, vty, V->ts);

  // Load first set of tiles
  int index;
  for (int x = 0; x < V->B->vtx; x++) {
    for (int y = 0; y < V->B->vty; y++) {
      index = x * V->B->vty + y;
      bufferLoadImage(V->S->osr, V->l, V->x + x, V->y + y, V->ts,
                      V->S->downsamples[V->l], V->B->bufs[index]);
      bufferProvisionImage(index + 1, V->ts, V->ts, V->B->bufs[index],
                           V->B->buf64);
      // Here offset is 0, so just put tx = x and ty = y
      viewerSetBufferIndices(V, index, x, y, V->ts, x, y);
    }
  }

  // Dirty for first render, show thumbnail
  V->dirty = 1;
  V->thumbnail_visible = 1;
}

void viewerToggleThumbnail(ViewerState *V) {
  V->thumbnail_visible = !V->thumbnail_visible;
  if (V->thumbnail_visible) {
    bufferDisplayImage(THUMBNAIL_ID, 0, 0, 0, 0, 2);
  } else {
    bufferClearImage(THUMBNAIL_ID);
  }
}

void viewerResetLevel(ViewerState *V) {
  V->ww = V->S->level_w[V->l];
  V->wh = V->S->level_h[V->l];
  V->mx = (int)floor((float)V->ww / V->ts);
  V->my = (int)floor((float)V->wh / V->ts);

  // Calculate ratio of old to new levels
  float ratio = V->S->downsamples[V->ol] / V->S->downsamples[V->l];
  V->x = V->x * ratio;
  V->y = V->y * ratio;
}

void viewerRender(ViewerState *V) {
  int ii, vx, vy, row, col, X, Y, index;
  int ts = V->ts;

  // For each tile, place according to buffer
  for (int x = 0; x < V->B->vtx; x++) {
    for (int y = 0; y < V->B->vty; y++) {
      index = x * V->B->vty + y; // random tile
      vx = V->B->vx[index];      // expected position in view
      vy = V->B->vy[index];
      ii = V->B->ii[index]; // expected kitty id

      // recompute params to send to kitty
      col = (vx * ts) / V->cw;
      row = (vy * ts) / V->ch;
      X = vx * ts - (col * V->cw);
      Y = vy * ts - (row * V->ch);

      // clear prev tile and position
      bufferClearImage(ii);

      // put tile with kitty id at current position in view
      bufferDisplayImage(ii, row, col, X, Y, -1);

      // Reset the old positions
      V->B->owx[index] = V->B->wx[index];
      V->B->owy[index] = V->B->wy[index];
    }
  }
}

void viewerRefreshScreen(ViewerState *V) {
  if (!V->dirty)
    return;

  // ... main render function ...
  viewerRender(V);

  // Initialize a buffer
  // realloc each time we append??
  // why not decent initial size and double like arrays??
  struct abuf ab = ABUF_INIT;

  // clear screen, move to origin, hide cursor
  abAppend(&ab, "\x1b[J", 3);
  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25l", 6);

  // Print debug info
  // viewerPrintDebug(V, &ab);

  // Finally write out
  if (write(STDOUT_FILENO, ab.b, ab.len) < 0)
    die("ab Buffer write\n");
  abFree(&ab);

  // Reset dirty state
  V->dirty = 0;
}

void viewerHandleKeypress(ViewerState *V, int key) {
  int newval;
  switch (key) {
  case 'h':
  case ARROW_LEFT:
    newval = MAX(V->x - 1, 0);
    if (newval != V->x) {
      V->ox = V->x;
      V->x = newval;
      viewerMoveLeft(V);
      V->dirty = 1;
    }
    break;
  case 'l':
  case ARROW_RIGHT:
    newval = MIN(V->x + 1, V->mx - 1);
    if (newval != V->x) {
      V->ox = V->x;
      V->x = newval;
      viewerMoveRight(V);
      V->dirty = 1;
    }
    break;
  case 'k':
  case ARROW_UP:
    newval = MAX(V->y - 1, 0);
    if (newval != V->y) {
      V->oy = V->y;
      V->y = newval;
      viewerMoveUp(V);
      V->dirty = 1;
    }
    break;
  case 'j':
  case ARROW_DOWN:
    newval = MIN(V->y + 1, V->my - 1);
    if (newval != V->y) {
      V->oy = V->y;
      V->y = newval;
      viewerMoveDown(V);
      V->dirty = 1;
    }
    break;
  case 'i':
    newval = MAX(V->l - 1, 0);
    if (newval != V->l) {
      V->ol = V->l;
      V->l = newval;
      viewerResetLevel(V);
      viewerZoomIn(V);
      V->dirty = 1;
    }
    break;
  case 'o':
    newval = MIN(V->l + 1, V->ml - 1);
    if (newval != V->l) {
      V->ol = V->l;
      V->l = newval;
      viewerResetLevel(V);
      viewerZoomOut(V);
      V->dirty = 1;
    }
    break;
  case 't':
    viewerToggleThumbnail(V);
    break;
  }
}

void viewerProcessKeypress(ViewerState *V) {
  int c = getKeypress();
  switch (c) {
  // Quit
  case 'q':
  case (CTRL_KEY('q')):
    clearScreen();
    moveCursor(0, 0);
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
  case 't':
    viewerHandleKeypress(V, c);
    break;
  }
}

void viewerSetBuffer(ViewerState *V, int index, int tx, int ty, int ts) {
  // load right tile into left's index
  bufferLoadImage(V->S->osr, V->l, tx, ty, ts, V->S->downsamples[V->l],
                  V->B->bufs[index]);
  bufferProvisionImage(index + 1, ts, ts, V->B->bufs[index], V->B->buf64);
}

void viewerSetBufferIndices(ViewerState *V, int index, int tx, int ty, int ts,
                            int x, int y) {
  V->B->ll[index] = V->l; // level of tile
  V->B->wx[index] = tx;   // pos of tile in slide
  V->B->wy[index] = ty;
  V->B->ii[index] = index + 1; // kitty index
  V->B->vx[index] = x;         // pos of tile in view
  V->B->vy[index] = y;
  // NOTE: We do not update owx, owy here,
  // rather after render

  // NOTE: Also, tried displaying here immediately,
  // but failed for some reason ( `ts` was used there )
}

void viewerMoveRight(ViewerState *V) {
  // Load new tiles into right column
  int tx, ty, x, y;
  int ts = V->B->ts;
  // using `ox` instead of `x` to get x from frame before refresh
  int roright = V->ox + V->B->vtx; // right of right column
  int left = V->ox;                // left column
  // iterate over tiles to find and replace
  for (int b = 0; b < V->B->vtx * V->B->vty; b++) {
    tx = V->B->owx[b];
    ty = V->B->owy[b];
    if (tx == left) {
      // replace left with right of right
      viewerSetBuffer(V, b, roright, ty, ts);

      // display on right column
      x = V->B->vtx - 1;
      y = V->B->vy[b];
      viewerSetBufferIndices(V, b, roright, ty, ts, x, y);
    } else {
      // shift to left
      x = V->B->vx[b] - 1;
      y = V->B->vy[b];
      viewerSetBufferIndices(V, b, tx, ty, ts, x, y);
    }
  }
}

void viewerMoveLeft(ViewerState *V) {
  // Load new tiles into right column
  int tx, ty, x, y;
  int ts = V->B->ts;
  // using `ox` instead of `x` to get x from frame before refresh
  int right = V->ox + V->B->vtx - 1; // right column
  int loleft = V->ox - 1;            // left of left column
  // iterate over tiles to find and replace
  for (int b = 0; b < V->B->vtx * V->B->vty; b++) {
    tx = V->B->owx[b];
    ty = V->B->owy[b];
    if (tx == right) {
      // replace right with left of left
      viewerSetBuffer(V, b, loleft, ty, ts);

      // display on left column
      x = 0;
      y = V->B->vy[b];
      viewerSetBufferIndices(V, b, loleft, ty, ts, x, y);
    } else {
      // shift to right
      x = V->B->vx[b] + 1;
      y = V->B->vy[b];
      viewerSetBufferIndices(V, b, tx, ty, ts, x, y);
    }
  }
}

void viewerMoveDown(ViewerState *V) {
  // Load new tiles into top row
  int tx, ty, x, y;
  int ts = V->B->ts;
  // using `oy` instead of `y` to get y from frame before refresh
  int bobot = V->oy + V->B->vty; // bottom of bottom row
  int top = V->oy;               // top row
  // iterate over tiles to find and replace
  for (int b = 0; b < V->B->vtx * V->B->vty; b++) {
    tx = V->B->owx[b];
    ty = V->B->owy[b];
    if (ty == top) {
      // replace top with bottom of bottom
      viewerSetBuffer(V, b, tx, bobot, ts);

      // display on bottom row
      x = V->B->vx[b];
      y = V->B->vty - 1;
      viewerSetBufferIndices(V, b, tx, bobot, ts, x, y);
    } else {
      // shift up
      x = V->B->vx[b];
      y = V->B->vy[b] - 1;
      viewerSetBufferIndices(V, b, tx, ty, ts, x, y);
    }
  }
}

void viewerMoveUp(ViewerState *V) {
  // Load new tiles into top row
  int tx, ty, x, y;
  int ts = V->B->ts;
  // using `oy` instead of `y` to get y from frame before refresh
  int bot = V->oy + V->B->vty - 1; // bottom row
  int totop = V->oy - 1;           // top of top row
  // iterate over tiles to find and replace
  for (int b = 0; b < V->B->vtx * V->B->vty; b++) {
    tx = V->B->owx[b];
    ty = V->B->owy[b];
    if (ty == bot) {
      // replace bottom with top of top
      viewerSetBuffer(V, b, tx, totop, ts);

      // display on top row
      x = V->B->vx[b];
      y = 0;
      viewerSetBufferIndices(V, b, tx, totop, ts, x, y);
    } else {
      // shift down
      x = V->B->vx[b];
      y = V->B->vy[b] + 1;
      viewerSetBufferIndices(V, b, tx, ty, ts, x, y);
    }
  }
}

void viewerZoomIn(ViewerState *V) {
  // Load new tiles everywhere
  int tx, ty, ts, x, y, b;
  ts = V->B->ts;
  tx = V->x;
  ty = V->y;
  // iterate over buffers and replace
  for (x = 0; x < V->B->vtx; x++) {
    for (y = 0; y < V->B->vty; y++) {
      b = x * V->B->vty + y;
      viewerSetBuffer(V, b, tx + x, ty + y, ts);
      viewerSetBufferIndices(V, b, tx + x, ty + y, ts, x, y);
    }
  }
}

void viewerZoomOut(ViewerState *V) {
  // Load new tiles everywhere
  int tx, ty, ts, x, y, b;
  ts = V->B->ts;
  tx = V->x;
  ty = V->y;
  // iterate over buffers and replace
  for (x = 0; x < V->B->vtx; x++) {
    for (y = 0; y < V->B->vty; y++) {
      b = x * V->B->vty + y;
      viewerSetBuffer(V, b, tx + x, ty + y, ts);
      viewerSetBufferIndices(V, b, tx + x, ty + y, ts, x, y);
    }
  }
}
