#include "cache.h"

#define MARGIN 2
#define TILE_NOT_LOADED -1
#define TILE_READ_ERROR -2

void cacheInit(Cache *C, openslide_t *osr, int ts, int cw, int ch, int aox,
               int aoy) {
  // Refs to slide, tile size to load tiles
  C->osr = osr;
  C->ts = ts;
  C->cw = cw;
  C->ch = ch;
  C->aox = aox;
  C->aoy = aoy;

  // Nothing is loaded yet
  C->levels[0] = -1;
  C->levels[1] = -1;
  C->levels[2] = -1;
}

void cacheLayerInit(Cache *C, int layer, int level, float downsample, int smi,
                    int smj, int vmi, int vmj, int left, int top) {
  // If there is already something free it?
  if (C->levels[layer] != -1)
    cacheLayerFree(C, layer);

  // Set reference to level
  C->levels[layer] = level;

  // Set level params
  LayerCache lc;
  lc.level = level;           // Level
  lc.downsample = downsample; // downsample
  lc.smi = smi;               // slide max coords for level
  lc.smj = smj;               //
  lc.vmi = vmi;               // view max coords
  lc.vmj = vmj;               //

  lc.left = left;        // Inclusive
  lc.top = top;          //
  lc.right = left + vmi; // Exclusive
  lc.bottom = top + vmj; //

  // Allocate the memory
  int num_pixels = C->ts * C->ts;
  lc.buf = (uint32_t *)malloc(num_pixels * sizeof(uint32_t));
  // + 1 for null terminator
  lc.buf64 = (char *)malloc((num_pixels * 4 + 1) * sizeof(char));

  // Set layer offset
  lc.koffset = layer * 10000;

  // First, visible tiles
  int index;
  int si, sj;
  for (int i = 0; i < vmi; i++) {
    for (int j = 0; j < vmj; j++) {
      // Constant definition for index
      index = (layer * vmi * vmj) + i * vmj + j;

      // Get si and sj
      si = i + left;
      sj = j + top;
      lc.si[index] = si;
      lc.sj[index] = sj;
      lc.kid[index] = TILE_NOT_LOADED;

      // Initialize an index
      int32_t kid = lc.koffset + index;

      // request tile from openslide, send to kitty
      int x = lc.si[index] * C->ts * lc.downsample;
      int y = lc.sj[index] * C->ts * lc.downsample;
      openslide_read_region(C->osr, lc.buf, x, y, level, C->ts, C->ts);
      if (openslide_get_error(C->osr) == NULL) {
        // clear old, if error, so be it
        kittyClearImage(kid);

        // encode and send to kitty, register kid
        RGBAtoRGBbase64(num_pixels, lc.buf, lc.buf64);
        kittyProvisionImage(kid, C->ts, C->ts, lc.buf64);
        lc.kid[index] = kid;
      } else {
        lc.kid[index] = TILE_READ_ERROR;
      }
    }
  }

  // Save the reference to initialized thang
  C->layers[layer] = lc;
}

void cacheDisplayLevel(Cache *C, int level) {
  int layer = cacheGetLayerOfLevel(C, level);
  if (layer < 0)
    return;
  LayerCache lc = C->layers[layer];

  int index, vx, vy, col, row, X, Y;
  for (int i = 0; i < lc.vmi; i++) {
    for (int j = 0; j < lc.vmj; j++) {
      // Constant definition for index
      index = (layer * lc.vmi * lc.vmj) + i * lc.vmj + j;

      // Move cursor to row, col, use X, Y as cell offset
      vx = i * C->ts + C->aox;
      vy = j * C->ts + C->aoy;
      col = vx / C->cw;
      row = vy / C->ch;
      X = vx - (col * C->cw);
      Y = vy - (row * C->ch);
      if (lc.kid[index] > 0)
        kittyDisplayImage(lc.kid[index], row, col, X, Y, -2);
    }
  }
}

int cacheGetLayerOfLevel(Cache *C, int level) {
  for (int i = 0; i < LAYERS; i++) {
    if (C->levels[i] == level)
      return i;
  }
  // Return -1 if not found
  return -1;
}

void cacheLayerFree(Cache *C, int layer) {
  // Free only if loaded
  if (C->levels[layer] < 0)
    return;
  // free(C->layers[layer]->buf); // segfaults for some reason
  // free(C->layers[layer]->buf64);
}

void cacheFree(Cache *C) {
  cacheLayerFree(C, 0);
  cacheLayerFree(C, 1);
  cacheLayerFree(C, 2);
}
