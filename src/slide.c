#include "slide.h"

void slideInit(SlideState *S, char *slide) {
  // Open ( exit if error )
  openslide_t *osr = openslide_open(slide);
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

  // Get associated images
  const char *const *associated_image_names =
      openslide_get_associated_image_names(osr);

  while (*associated_image_names) {
    int64_t w, h;
    const char *name = *associated_image_names;

    if (!strcmp(name, "thumbnail\0")) {
      // Get size of thumbnail
      openslide_get_associated_image_dimensions(osr, name, &w, &h);

      // Allocate
      S->thumbnail_w = w;
      S->thumbnail_h = h;
      S->thumbnail = malloc(h * w * sizeof(uint32_t));

      // Read thumbnail
      openslide_read_associated_image(osr, name, S->thumbnail);
      return;
    }

    associated_image_names++;
  }
}

void slideFree(SlideState *S) {
  openslide_close(S->osr);
  free(S->thumbnail);
}
