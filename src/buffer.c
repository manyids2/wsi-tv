#include "buffer.h"
#include <stdlib.h>

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); }

void bufferInit(BufferState *B, int mx, int my, int ts) {
  B->mx = mx;
  B->my = my;
  B->ts = ts;

  // Allocate memory for pointers to buffers
  int max_buffers = mx * my;
  B->bufs = calloc(max_buffers, sizeof(int *));

  // Initialize values, memory at each tile position
  int index;
  for (int x = 0; x < mx; x++) {
    for (int y = 0; y < my; y++) {
      // Formula for index
      index = x * my + y;

      // Initialize ids of tiles at bufs[index]
      B->ll[index] = 0;
      B->xx[index] = 0;
      B->yy[index] = 0;

      // Allocate memory for one tile
      B->bufs[index] = malloc(B->ts * B->ts * 4 * sizeof(uint32_t));
    }
  }
}

void bufferFree(BufferState *B) {
  for (int i = 0; i < B->mx * B->my; i++) {
    free(&B->bufs[i]);
  }
}
