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
  getWindowSize(&V->rows, &V->cols, &V->vw, &V->vh);
  getWindowSizeKitty(&V->vw, &V->vh);

  // Compute cell dims
  V->cw = (int)V->vw / V->cols;
  V->ch = (int)V->vh / V->rows;

  // Initialize view level maximums
  V->vmi = V->vw / V->ts;
  V->vmj = V->vh / V->ts;

  // Centering offset
  V->ox = (V->vw - (V->vmi * V->ts)) / 2;
  V->oy = (V->vh - (V->vmj * V->ts)) / 2;

  // Lets start from level 0
  viewSetLevel(V, 0);

  // Center slide on screen
  V->wx = V->ww / 2;
  V->wy = V->wh / 2;
  viewSetWorldPosition(V, V->wx, V->wy);
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
  V->si = V->sw / V->ts;
  V->sj = V->sh / V->ts;
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

  // Set old also to same
  V->ol = V->l;
  V->osi = V->si;
  V->osj = V->sj;
}

void viewPrintDebug(View *V) {
  if (!V->debug) {
    return;
  }
  char s[512];
  int len;
  len = snprintf(s, sizeof(s),
                 "View: \r\n"
                 "  Slide level constants: \r\n"
                 "            ts: %6d \r\n"
                 "      ww,   wh: %6ld, %6ld \r\n"
                 "  View level constants: \r\n"
                 "    cols, rows: %6d, %6d \r\n"
                 "      vw,   vh: %6d, %6d \r\n"
                 "      cw,   ch: %6d, %6d \r\n"
                 "      vx,   vy: %6d, %6d \r\n"
                 "     vmi,  vmj: %6d, %6d \r\n"
                 "      ox,   oy: %6d, %6d \r\n",
                 V->ts, V->ww, V->wh, V->cols, V->rows, V->vw, V->vh, V->cw,
                 V->ch, V->vx, V->vy, V->vmi, V->vmj, V->ox, V->oy);
  assert(write(STDOUT_FILENO, s, len) >= 0);
}

void viewFree(View *V) { slideFree(V->S); }
