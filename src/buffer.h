#include <openslide/openslide.h> // for uint32_t
#include <stdlib.h>
#include <string.h>

#define MAX_XY 32

struct abuf {
  char *b;
  int len;
};
#define ABUF_INIT                                                              \
  { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

typedef struct BufferState {
  int mx, my, ts;  // size in x, y; tile size
  uint32_t **bufs; // pointers to actual buffers
  int ll[MAX_XY];  // level, x, y of buf_ij
  int xx[MAX_XY];
  int yy[MAX_XY];
} BufferState;

void bufferInit(BufferState *B, int mx, int my, int ts);
void bufferFree(BufferState *B);
