#include "viewer.h"

void viewerFree(ViewerState *V) {
  // nothing to free yet
  //
  bufferFree(V->B);
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

void viewerInit(ViewerState *V) {
  // Clear screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // Read window size
  if (getWindowSize(&V->rows, &V->cols, &V->vw, &V->vh) == -1)
    die("getWindowSize");

  // Compute cell size
  V->cw = (int)V->vw / V->cols;
  V->ch = (int)V->vh / V->rows;

  // Store current l, x, y, world, view
  V->l = V->S->level_count - 1; // Lowest zoom
  V->x = 0;                     // Starting tile
  V->y = 0;                     //  at top left corner
  //
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
  // Put tile in first buffer
  int ix, iy, row, col, X, Y, index;
  int ts = V->ts;

  // Put tile in buffer
  for (int x = 0; x < V->B->vtx; x++) {
    for (int y = 0; y < V->B->vty; y++) {
      index = x * V->B->vty + y;
      ix = V->B->ix[index];
      iy = V->B->iy[index];
      col = (ix * ts) / V->cw;
      row = (iy * ts) / V->ch;
      X = ix * ts - (col * V->cw);
      Y = iy * ts - (row * V->ch);
      bufferClearImage(index + 1);
      bufferDisplayImage(index + 1, row, col, X, Y, -1);
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
  viewerPrintDebug(V, &ab);

  // Finally write out
  write(STDOUT_FILENO, ab.b, ab.len);
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
      V->dirty = 1;
    }
    break;
  case 'o':
    newval = MIN(V->l + 1, V->ml - 1);
    if (newval != V->l) {
      V->ol = V->l;
      V->l = newval;
      viewerResetLevel(V);
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
  case 't':
    viewerHandleKeypress(V, c);
    break;
  }
}

void viewerSetBuffer(ViewerState *V, int index, int tx, int ty, int ts, int x,
                     int y) {
  // load right tile into left's index
  bufferLoadImage(V->S->osr, V->l, tx, ty, ts, V->S->downsamples[V->l],
                  V->B->bufs[index]);
  bufferProvisionImage(index + 1, ts, ts, V->B->bufs[index], V->B->buf64);

  // record l, x, y, kitty id at buffer position index
  V->B->ll[index] = V->l;
  V->B->xx[index] = tx;
  V->B->yy[index] = ty;
  V->B->ii[index] = index + 1;
  V->B->ix[index] = x;
  V->B->iy[index] = y;
}

void viewerSetBufferIndices(ViewerState *V, int index, int tx, int ty, int x,
                            int y) {
  V->B->ll[index] = V->l;
  V->B->xx[index] = tx;
  V->B->yy[index] = ty;
  V->B->ii[index] = index + 1;
  V->B->ix[index] = x;
  V->B->iy[index] = y;
}

void viewerMoveLeft(ViewerState *V) {
  // Load new tiles into right column
  int index, ty;
  int x = V->B->vtx - 1; // right column
  int ts = V->B->ts;
  int tx = V->x - 1; // before left tile
  // iterate over rows
  for (int y = 0; y < V->B->vty; y++) {
    index = x * V->B->vty + y; // right tile
    ty = V->y + y;
    viewerSetBuffer(V, index, tx, ty, ts, x, y);
  }
  // shift the rest
  for (x = 0; x < V->B->vtx - 1; x++) {
    for (int y = 0; y < V->B->vty; y++) {
      index = x * V->B->vty + y;
      tx = V->x + x;
      ty = V->y + y;
      viewerSetBufferIndices(V, index, tx, ty, x + 1, y);
    }
  }
}

void viewerMoveRight(ViewerState *V) {
  // Load new tiles into left column
  int index, ty;
  int x = 0; // left column
  int ts = V->B->ts;
  int tx = V->x + V->B->vtx; // After right tile
  // iterate over rows
  for (int y = 0; y < V->B->vty; y++) {
    index = x * V->B->vty + y; // left tile
    ty = V->y + y;
    viewerSetBuffer(V, index, tx, ty, ts, x, y);
  }
  // shift the rest
  for (x = 1; x < V->B->vtx; x++) {
    for (int y = 0; y < V->B->vty; y++) {
      index = x * V->B->vty + y;
      tx = V->x + x;
      ty = V->y + y;
      viewerSetBufferIndices(V, index, tx, ty, x - 1, y);
    }
  }
}

void viewerMoveUp(ViewerState *V) {
  // Load new tiles into bottom row
  int index, tx;
  int y = V->B->vty - 1; // bottom row
  int ts = V->B->ts;
  int ty = V->y - 1; // above up tile
  // iterate over rows
  for (int x = 0; x < V->B->vtx; x++) {
    index = x * V->B->vty + y; // bottom tile
    tx = V->x + x;
    viewerSetBuffer(V, index, tx, ty, ts, x, y);
  }
  // shift the rest
  for (int x = 0; x < V->B->vtx; x++) {
    for (y = 0; y < V->B->vty - 1; y++) {
      index = x * V->B->vty + y;
      tx = V->x + x;
      ty = V->y + y;
      viewerSetBufferIndices(V, index, tx, ty, x, y - 1);
    }
  }
}

void viewerMoveDown(ViewerState *V) {
  // Load new tiles into up row
  int index, tx;
  int y = 0; // up row
  int ts = V->B->ts;
  int ty = V->y + V->B->vty; // below bottom tile
  // iterate over rows
  for (int x = 0; x < V->B->vtx; x++) {
    index = x * V->B->vty + y; // up tile
    tx = V->x + x;
    viewerSetBuffer(V, index, tx, ty, ts, x, y);
  }

  // shift the rest
  for (int x = 0; x < V->B->vtx; x++) {
    for (y = 1; y < V->B->vty; y++) {
      index = x * V->B->vty + y;
      tx = V->x + x;
      ty = V->y + y;
      viewerSetBufferIndices(V, index, tx, ty, x, y + 1);
    }
  }
}
