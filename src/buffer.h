#include <assert.h>
#include <libpng/png.h>
#include <math.h>
#include <openslide/openslide.h> // for uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFERS 32 * 32 // max of 32 rows and cols
#define TILE_SIZE 256
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CHUNK 4096 // for kitty

struct abuf {
  char *b;
  int len;
};
#define ABUF_INIT                                                              \
  { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

typedef struct BufferState {
  int mx, my, ts;      // size in x, y; tile size
  uint32_t **bufs;     // pointers to actual buffers
  int ll[MAX_BUFFERS]; // level, x, y of buf_ij
  int xx[MAX_BUFFERS];
  int yy[MAX_BUFFERS];
} BufferState;

void bufferInit(BufferState *B, int mx, int my, int ts);
void bufferFree(BufferState *B);

// image related
void bufferLoadImage(openslide_t *osr, int l, int tx, int ty, int ts,
                     float downsample, uint32_t *buf);
void bufferProvisionImage(int index, int w, int h, uint32_t *buf);
void bufferDisplayImage(int index, int row, int col, int X, int Y, int Z);
void bufferClearImage(int index);
void bufferDeleteImage(int index);
