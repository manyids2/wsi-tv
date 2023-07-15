#include "view.h"

void viewInit(View *V, char *slide) {
  V->dirty = 1;
  V->debug = 1;

  // Initialize slide
  slideInit(V->S, slide);

  // Always constants
  V->ts = TILE_SIZE;
  V->ww = V->S->level_w[V->S->level_count - 1];
  V->wh = V->S->level_h[V->S->level_count - 1];

  // Query to get sizes
  getWindowSize(&V->rows, &V->cols, &V->vtw, &V->vth);
  getWindowSizeKitty(&V->vtw, &V->vth);

  // Compute cell dims
  V->cw = (int)V->vw / V->cols;
  V->ch = (int)V->vh / V->rows;

  // 10 chars offset in case of sidebar
  V->ox = V->cw * 10;
  V->oy = 0;

  // Compute effective view
  V->vw = V->vtw - V->ox;
  V->vh = V->vth - V->oy;

  // Initialize view level maximums
  V->vmi = V->vw / V->ts;
  V->vmj = V->vh / V->ts;

  // Lets start from level 0
  viewSetLevel(V, 0);

  // Center slide on screen
  V->wx = V->ww / 2;
  V->wy = V->wh / 2;
  viewSetWorldPosition(V, V->wx, V->wy);

  // Set old also to same
  V->ol = V->l;
  V->osi = V->si;
  V->osj = V->sj;
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
}

void viewSetWorldPosition(View *V, int64_t wx, int64_t wy) {
  V->wx = wx;
  V->wy = wy;

  // Corresponding slide positions
  V->sx = V->wx / V->downsample;
  V->sy = V->wy / V->downsample;

  // Recompute top left
  // NOTE: edge case -> level is too small for view
  V->si = MAX((V->sx - (V->vw / 2)) / V->ts, 0);
  V->sj = MAX((V->sy - (V->vh / 2)) / V->ts, 0);
}

void viewSetSlidePosition(View *V, int64_t sx, int64_t sy) {
  // Corresponding world position
  V->wx = sx * V->downsample;
  V->wy = sy * V->downsample;

  // Set slide positions using world
  viewSetWorldPosition(V, V->wx, V->wy);
}

void viewMoveUp(View *V) {
  V->sy = MAX(V->sy - V->ts, V->ts); // Keep margin of ts from center
  viewSetSlidePosition(V, V->sx, V->sy);
}
void viewMoveDown(View *V) {
  V->sy = MIN(V->sy + V->ts, V->sh - V->ts); // Keep margin of ts from center
  viewSetSlidePosition(V, V->sx, V->sy);
}
void viewMoveLeft(View *V) {
  V->sx = MAX(V->sx - V->ts, V->ts); // Keep margin of ts from center
  viewSetSlidePosition(V, V->sx, V->sy);
}
void viewMoveRight(View *V) {
  V->sx = MIN(V->sx + V->ts, V->sw - V->ts); // Keep margin of ts from center
  viewSetSlidePosition(V, V->sx, V->sy);
}
void viewZoomIn(View *V) {
  V->l = MAX(V->l - 1, 0);
  viewSetLevel(V, V->l);
}
void viewZoomOut(View *V) {
  V->l = MIN(V->l + 1, V->S->level_count - 1);
  viewSetLevel(V, V->l);
}

void viewDrawTiles(View *V) {
  char s[32];
  int len;
  static const int rs = 5;  // row spacing
  static const int cs = 10; // column spacing
  for (int j = 0; j < V->vmj; j++) {
    for (int i = 0; i < V->vmi; i++) {
      moveCursor(j * rs, i * cs);
      len = snprintf(s, sizeof(s), "%d,%d", i, j);
      assert(write(STDOUT_FILENO, s, len) >= 0);
    }
    assert(write(STDOUT_FILENO, "\r\n", 4) >= 0);
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
                   "   Old:                    \r\n"
                   "            ol: %6d        \r\n"
                   "     osi,  osj: %6d, %6d   \r\n"
                   "",
                   V->ts, V->ww, V->wh, V->cols, V->rows, V->vw, V->vh, V->cw,
                   V->ch, V->vx, V->vy, V->vmi, V->vmj, V->ox, V->oy, V->sw,
                   V->sh, V->smi, V->smj, V->downsample, V->l, V->si, V->sj,
                   V->wx, V->wy, V->sx, V->sy, V->ol, V->osi, V->osj);
    assert(write(STDOUT_FILENO, s, len) >= 0);
    break;
  case 2:
    viewDrawTiles(V);
    break;
  default:
    break;
  }
}

void viewFree(View *V) { slideFree(V->S); }
