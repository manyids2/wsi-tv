#include <assert.h>
#include <openslide/openslide.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEVELS 32
#define MAX_ASSOCIATED_IMAGES 8
#define THUMBNAIL_ID 128000 // arbit large int for kitty

typedef struct Slide {
  // Openslide slide
  openslide_t *osr;

  // Slide properties
  int level_count;
  int64_t level_w[MAX_LEVELS];
  int64_t level_h[MAX_LEVELS];
  float downsamples[MAX_LEVELS];

  // Thumbnail
  uint32_t *thumbnail;              // buffer
  int64_t thumbnail_w, thumbnail_h; // dims
} Slide;

// Constructor and destructor
void slideInit(Slide *S, char *slide);
void slideFree(Slide *S);

// Get (i, j)th tile from level (l), given tile size (ts)
void slideGetTile(Slide *S, uint32_t *buf, int l, int i, int j, int ts);
