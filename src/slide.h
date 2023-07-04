#include <assert.h>
#include <stdlib.h>
#include <openslide/openslide.h>

#define MAX_LEVELS 32

typedef struct SlideState {
  // Openslide slide
  char *slidename;
  openslide_t *osr;
  
  // Slide properties
  int level_count;
  int64_t level_w[MAX_LEVELS];
  int64_t level_h[MAX_LEVELS];
  float downsamples[MAX_LEVELS];
} SlideState;

void slideInit(SlideState *ss, char * path);
void slideFree(SlideState *ss);
