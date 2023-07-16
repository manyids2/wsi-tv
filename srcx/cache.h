#include <assert.h>
#include <stdint.h>

// Strucure of buffer
#define LAYERS 3

// limit view to 3072x2048, define landscape?
#define TILE_SIZE 256
#define MAX_I 12
#define MAX_J 8

// limit similar to kitty limits (320MB)
#define MAX_BUFFERS_PER_LAYER 96 * 2
#define MAX_BUFFERS 96 * 6

typedef struct LayerCache {
  // Level
  int level;

  // Maximums
  int smi, smj, vmi, vmj;

  // slide level tile indices
  int left, right, top, bottom;

  // buffers
  uint32_t *bufs[MAX_BUFFERS_PER_LAYER];

  // buffer si, sj of tiles in buffers;
  int *si[MAX_BUFFERS_PER_LAYER];
  int *sj[MAX_BUFFERS_PER_LAYER];

  // Kitty id -> will use for locks
  // NOTE: kid[..] == -1  => not loaded
  //       kid[..] ==  0  => loading
  //       kid[..] >=  1  => loaded
  int32_t *kid[MAX_BUFFERS_PER_LAYER];
} LayerCache;

typedef struct Cache {
  // Buffers for each level
  int levels[LAYERS]; // Layer holds which level
  LayerCache *layers[LAYERS];
} Cache;

void cacheInit(Cache *C);
void cacheInitLayer(Cache *C, int layer, int level, int smi, int smj, int vmi,
                    int vmj, int left, int top);

int cacheGetLayerOfLevel(Cache *C, int level);

void cacheFreeLayer(Cache *C, int layer);
void cacheFree(Cache *C);
