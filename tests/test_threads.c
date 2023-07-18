#include <assert.h>
#include <openslide/openslide.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define TILE_SIZE 1024

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: ./test_threads slide num_tiles num_threads\n");
    return EXIT_FAILURE;
  }

  // Get path to slide
  char *slide = argv[1];
  int num_tiles = atoi(argv[2]);
  int num_threads = atoi(argv[3]);
  printf("slidepath: %s\n"
         "  num_tiles  : %d \n"
         "  num_threads: %d \n",
         slide, num_tiles, num_threads);

  // Open slide
  openslide_t *osr = openslide_open(slide);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  // Allocate buffer
  uint32_t *buf = malloc(TILE_SIZE * TILE_SIZE * sizeof(uint32_t));

  // Single threaded
  int level = 0;

  // Read first tile many times ( so should not factor into timings )
  for (int i = 1; i < num_tiles; i++) {
    openslide_read_region(osr, buf, 0, 0, level, TILE_SIZE, TILE_SIZE);
  }

  // Free buffer
  free(buf);

  // Close slide
  openslide_close(osr);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  return EXIT_SUCCESS;
}
