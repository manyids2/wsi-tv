#include "cache.h"

#define MARGIN 2

void cacheInit(Cache *C, openslide_t *osr, int ts) {
  C->osr = osr;
  C->ts = ts;

  // Order of levels should not matter
  C->levels[0] = -1;
  C->levels[1] = -1;
  C->levels[2] = -1;
}

void cacheInitLayer(Cache *C, int layer, int level, float downsample, int smi,
                    int smj, int vmi, int vmj, int left, int top) {
  // If there is already something free it?
  if (C->levels[layer] != -1)
    cacheFreeLayer(C, layer);

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
  lc.buf64 = (char *)malloc(num_pixels * 4 * sizeof(char) + 1);

  // Set layer offset
  lc.koffset = layer * 100000;

  // First, visible tiles
  int index = 0;
  for (int i = 0; i < vmi; i++) {
    for (int j = 0; j < vmj; j++) {
      // Get si and sj
      *lc.si[index] = i + left;
      *lc.sj[index] = j + top;
      *lc.kid[index] = -1; // not loaded

      // Initialize an index
      int32_t kid = lc.koffset + index;

      // request tile from openslide
      int x = *lc.si[index] * C->ts * lc.downsample;
      int y = *lc.sj[index] * C->ts * lc.downsample;
      openslide_read_region(C->osr, lc.buf, x, y, level, C->ts, C->ts);
      if (openslide_get_error(C->osr) == NULL) {
        // Encode to base64
        RGBAtoRGBbase64(num_pixels, lc.buf, lc.buf64);

        // Send to kitty
        kittyProvisionImage(kid, C->ts, C->ts, lc.buf64);

        // Assuming successful, assign kitty id
        *lc.kid[index] = kid;
      } else {
        // Nothing to do really
      }

      // Increment index
      index++;

      // break inner
      if (index > MAX_TILES_PER_LAYER)
        break;
    }

    // break outer
    if (index > MAX_TILES_PER_LAYER)
      break;
  }

  // Then, first neighbours

  // Finally, second neighbours

  // Save the reference to initialized thang
  C->layers[layer] = &lc;
}

int cacheGetLayerOfLevel(Cache *C, int level) {
  for (int i = 0; i < LAYERS; i++) {
    if (C->levels[i] == level)
      return i;
  }
  // Return -1 if not found
  return -1;
}

void cacheFreeLayer(Cache *C, int layer) {
  if (C->levels[layer] < 0)
    return;
}

void cacheFree(Cache *C) {
  // Free if already loaded
  if (C->levels[0] >= 0)
    cacheFreeLayer(C, 0);
  if (C->levels[1] >= 1)
    cacheFreeLayer(C, 1);
  if (C->levels[2] >= 2)
    cacheFreeLayer(C, 2);
}
