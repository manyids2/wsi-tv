#include "view.h"

void viewInit(View *V, char *slide) {
  V->dirty = 1;
  V->debug = 1;

  // Initialize slide
  slideInit(V->S, slide);

  // Always constants
  V->ts = TILE_SIZE;
  V->ww = V->S->level_w[0];
  V->wh = V->S->level_h[0];

  // Query to get sizes
  getWindowSize(&V->rows, &V->cols, &V->vtw, &V->vth);
  getWindowSizeKitty(&V->vtw, &V->vth);

  // Compute cell dims
  V->cw = V->vtw / V->cols;
  V->ch = V->vth / V->rows;

  // Keep margin of 1 on top for status bar
  // 5 on left for style
  V->aox = 5 * V->cw;
  V->aoy = 15 * V->ch;

  // Compute effective view ( for now, no margin on bottom and right )
  // bound to make sure kitty can handle 3 layers
  V->vw = MIN(V->vtw - V->aox, TILE_SIZE * 12);
  V->vh = MIN(V->vth - V->aoy, TILE_SIZE * 8);

  // Initialize view level maximums
  V->vmi = V->vw / V->ts;
  V->vmj = V->vh / V->ts;

  // Lets start from least zoom
  viewSetLevel(V, V->S->level_count - 1);

  // Center slide on screen
  V->wx = V->ww / 2;
  V->wy = V->wh / 2;
  viewSetWorldPosition(V, V->wx, V->wy);

  // Initialize cache
  cacheInit(V->C, V->S->osr, V->ts, V->cw, V->ch, V->aox, V->aoy);

  // Store 3 consecutive layers, starting with least zoom
  int level, downsample, smi, smj, vmi, vmj, left, top;

  // Initialize cache layers
  clearScreen();
  for (int layer = 0; layer < 3; layer++) {
    level = V->l - layer;
    if (level < 0)
      break;
    downsample = V->S->downsamples[level];
    smi = V->S->level_w[level] / V->ts;
    smj = V->S->level_h[level] / V->ts;
    vmi = V->vmi;
    vmj = V->vmj;

    viewGetTileFromWorldPosition(V, V->wx, V->wy, level, &left, &top);

    cacheLayerInit(V->C, layer, level, downsample, smi, smj, vmi, vmj, left,
                   top);
  }

  // Show lowest zoom
  cacheDisplayLevel(V->C, V->l);
}

void viewSetLevel(View *V, int level) {
  // Set level
  V->l = level;

  // Store downsample factor
  V->downsample = V->S->downsamples[V->l];

  // Get dims
  V->sw = V->S->level_w[V->l];
  V->sh = V->S->level_h[V->l];

  // Compute maximums
  V->smi = V->sw / V->ts;
  V->smj = V->sh / V->ts;

  // Keep world position constant
  viewSetWorldPosition(V, V->wx, V->wy);

  // Update the cache
  // Handle the various case
  // - near top
  // - in the middle
  // - near bottom
  for (int layer = 0; layer < 3; layer++) {
    int level = V->l - layer;
    if (level < 0)
      break;
  }
}

void viewSetWorldPosition(View *V, int64_t wx, int64_t wy) {
  V->wx = wx;
  V->wy = wy;

  // Corresponding slide positions
  V->sx = V->wx / V->downsample;
  V->sy = V->wy / V->downsample;

  // Recompute top left
  // NOTE: edge case -> level is too small for view
  V->si = (V->sx - (V->vw / 2)) / V->ts;
  V->sj = (V->sy - (V->vh / 2)) / V->ts;

  // Reset to align tiles
  viewSetSlideCoords(V, V->si, V->sj);
}

void viewGetTileFromWorldPosition(View *V, int64_t wx, int64_t wy, int level,
                                  int *si, int *sj) {
  V->wx = wx;
  V->wy = wy;

  // Corresponding slide positions
  int64_t sx = V->wx / V->S->downsamples[level];
  int64_t sy = V->wy / V->S->downsamples[level];

  // Recompute top left
  // HACK: Why does this need +1??
  *si = ((sx - (V->vw / 2)) / V->ts) + 1;
  *sj = ((sy - (V->vh / 2)) / V->ts) + 1;
}

void viewSetSlideCoords(View *V, int si, int sj) {
  // Store new top left coords
  V->si = si;
  V->sj = sj;

  // Corresponding slide and world position
  V->sx = (si * V->ts) + (V->vw / 2);
  V->sy = (sj * V->ts) + (V->vh / 2);
  V->wx = V->sx * V->downsample;
  V->wy = V->sy * V->downsample;
}

void viewMoveUp(View *V) {
  int sj = MAX(V->sj - 1, 0);
  if (sj == V->sj)
    return;
  viewSetSlideCoords(V, V->si, sj);
}

void viewMoveDown(View *V) {
  int sj = MIN(V->sj + 1, V->smj - V->vmj);
  if (sj == V->sj)
    return;
  viewSetSlideCoords(V, V->si, sj);
}

void viewMoveLeft(View *V) {
  int si;
  si = MAX(V->si - 1, 0);
  if (si == V->si)
    return;
  viewSetSlideCoords(V, si, V->sj);
}

void viewMoveRight(View *V) {
  int si;
  si = MIN(V->si + 1, V->smi - V->vmi);
  if (si == V->si)
    return;
  viewSetSlideCoords(V, si, V->sj);
}

void viewZoomIn(View *V) {
  int l = MAX(V->l - 1, 0);
  if (l == V->l)
    return;
  viewSetLevel(V, l);
}

void viewZoomOut(View *V) {
  int l = MIN(V->l + 1, V->S->level_count - 1);
  if (l == V->l)
    return;
  viewSetLevel(V, l);
}

void viewDrawTiles(View *V) {
  char s[32];
  int len;
  int x, y, si, sj, row, col, X, Y;
  for (int j = 0; j < V->vmj; j++) {
    for (int i = 0; i < V->vmi; i++) {
      si = V->si + i;
      sj = V->sj + j;
      x = i * V->ts + V->aox;
      y = j * V->ts + V->aoy;
      col = x / V->cw;
      row = y / V->ch;
      X = x - (col * V->cw);
      Y = y - (row * V->ch);

      // Write current row, col
      moveCursor(row, col);
      len = snprintf(s, sizeof(s), "c,r:%d,%d", si, sj);
      assert(write(STDOUT_FILENO, s, len) >= 0);

      // Write current X, Y
      moveCursor(row + 1, col);
      len = snprintf(s, sizeof(s), "X,Y:%d,%d", X, Y);
      assert(write(STDOUT_FILENO, s, len) >= 0);
    }
  }
}

void viewDrawCache(View *V) {
  char s[32];
  int len, index;
  int32_t kid;
  LayerCache lc;
  int x, y, si, sj, row, col;
  for (int layer = 0; layer < 3; layer++) {
    lc = V->C->layers[layer];
    for (int j = 0; j < V->vmj; j++) {
      for (int i = 0; i < V->vmi; i++) {
        x = i * V->ts + V->aox;
        y = j * V->ts + V->aoy;
        col = x / V->cw;
        row = y / V->ch;

        index = (layer * V->vmi * V->vmj) + i * V->vmj + j;
        kid = lc.kid[index];
        si = lc.si[index];
        sj = lc.sj[index];

        // Write current kid
        moveCursor(row + layer * 2, col);
        len = snprintf(s, sizeof(s), "%d.kid: %d", lc.level, kid);
        assert(write(STDOUT_FILENO, s, len) >= 0);

        // Write current row, col
        moveCursor(row + 1 + layer * 2, col);
        len = snprintf(s, sizeof(s), "si,sj: %d,%d", si, sj);
        assert(write(STDOUT_FILENO, s, len) >= 0);
      }
    }
  }
}

void viewPrintDebug(View *V) {
  char s[2048];
  int len;
  switch (V->debug) {
  case 1:
    len = snprintf(s, sizeof(s),
                   "View:              \r\n"
                   "  Slide level constants:   \r\n"
                   "            ts: %6d        \r\n"
                   "      ww,   wh: %6ld, %6ld \r\n"
                   "  View level constants:    \r\n"
                   "    cols, rows: %6d, %6d   \r\n"
                   "      vw,   vh: %6d, %6d   \r\n"
                   "      cw,   ch: %6d, %6d   \r\n"
                   "      vx,   vy: %6d, %6d   \r\n"
                   "     vmi,  vmj: %6d, %6d   \r\n"
                   "      ox,   oy: %6d, %6d   \r\n"
                   "  Level constants:         \r\n"
                   "      sw,   sh: %6ld, %6ld \r\n"
                   "     smi,  smj: %6d, %6d   \r\n"
                   "    downsample: %6f        \r\n"
                   "  State:                   \r\n"
                   "         level: %6d        \r\n"
                   "      si,   sj: %6d, %6d   \r\n"
                   "      wx,   wy: %6ld, %6ld \r\n"
                   "      sx,   sy: %6ld, %6ld \r\n"
                   "",
                   V->ts, V->ww, V->wh, V->cols, V->rows, V->vw, V->vh, V->cw,
                   V->ch, V->vx, V->vy, V->vmi, V->vmj, V->aox, V->aoy, V->sw,
                   V->sh, V->smi, V->smj, V->downsample, V->l, V->si, V->sj,
                   V->wx, V->wy, V->sx, V->sy);
    assert(write(STDOUT_FILENO, s, len) >= 0);
    break;
  case 2:
    viewDrawTiles(V);
    break;
  case 3:
    viewDrawCache(V);
    break;
  default:
    break;
  }
}

void viewFree(View *V) {
  slideFree(V->S);
  cacheFree(V->C);
}
