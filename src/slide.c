#include "slide.h"

void slideInit(SlideState *S, char *path) {
  // Open ( exit if error )
  openslide_t *osr = openslide_open(path);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  // Get count of levels in wsi pyramid
  S->level_count = openslide_get_level_count(osr);
  assert(S->level_count < MAX_LEVELS);

  // Store downsamples ( scale factors ), level_w, level_h
  for (int32_t level = 0; level < openslide_get_level_count(osr); level++) {
    S->downsamples[level] = openslide_get_level_downsample(osr, level);
    openslide_get_level_dimensions(osr, level, &S->level_w[level],
                                   &S->level_h[level]);
  }
}

void slideFree(SlideState *S) { openslide_close(S->osr); }
