#include "kitty.h"
#include <assert.h>
#include <openslide/openslide.h>
#include <stdint.h>

// Strucure of buffer
#define LAYERS 3

// limit view to 3072x2048, define landscape?
#define TILE_SIZE 256
#define MAX_I 12
#define MAX_J 8

// limit similar to kitty limits (320MB)
#define MAX_TILES_PER_LAYER 192

typedef struct LayerCache {
  // Level
  int level;
  float downsample;

  // Maximums
  int smi, smj, vmi, vmj;

  // slide level tile indices
  int left, right, top, bottom;

  // buffers
  uint32_t *buf;
  char *buf64;
  int32_t koffset; // Offset for layer when storing kitty ids

  // buffer si, sj of tiles in buffers;
  int si[MAX_TILES_PER_LAYER];
  int sj[MAX_TILES_PER_LAYER];

  // Kitty id -> will use for locks
  // NOTE: kid[..] == -1  => not loaded
  //       kid[..] ==  0  => loading
  //       kid[..] >=  1  => loaded
  int32_t kid[MAX_TILES_PER_LAYER];
} LayerCache;

typedef struct Cache {
  // slide
  openslide_t *osr;
  int ts, cw, ch, aox, aoy; // Needed for drawing

  // Buffers for each level
  int levels[LAYERS]; // Layer holds which level
  LayerCache layers[LAYERS];
} Cache;

void cacheInit(Cache *C, openslide_t *osr, int ts, int cw, int ch, int aox,
               int aoy);
void cacheLayerInit(Cache *C, int layer, int level, float downsample, int smi,
                    int smj, int vmi, int vmj, int left, int top);

int cacheGetLayerOfLevel(Cache *C, int level);
void cacheDisplayLevel(Cache *C, int level);

void cacheLayerFree(Cache *C, int layer);
void cacheFree(Cache *C);
