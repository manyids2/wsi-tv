#include "cache.h"

#define MARGIN 2

void cacheInit(Cache *C) {
  // Order of levels should not matter
  C->levels[0] = -1;
  C->levels[1] = -1;
  C->levels[2] = -1;
}

void cacheInitLayer(Cache *C, int layer, int level, int smi, int smj, int vmi,
                    int vmj, int left, int top) {
  // If there is already something free it?
  if (C->levels[layer] != -1)
    cacheFreeLayer(C, layer);

  // Set reference to level
  C->levels[layer] = level;

  // Set level params
  LayerCache lc;
  lc.level = level; // Level
  lc.smi = smi;     // slide max coords for level
  lc.smj = smj;     //
  lc.vmi = vmi;     // view max coords
  lc.vmj = vmj;     //

  lc.left = left;        // Inclusive
  lc.top = top;          //
  lc.right = left + vmi; // Exclusive
  lc.bottom = top + vmj; //

  // First, visible tiles
  for (int i = 0; i < vmi; i++) {
    for (int j = 0; j < vmj; j++) {
      // Get si and sj
    }
  }

  // Then, first neighbours

  // Finally, second neighbours

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
