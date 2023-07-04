#include "buffer.h"
#include "slide.h"
#include "term.h"

#define TILE_SIZE 256
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct ViewerState {
  int dirty; // only render if dirty

  int cols, rows; // term size
  int ww, wh;     // world dims ( slide )
  int vw, vh;     // view dims ( screen )
  int cw, ch;     // cell dims

  int l, x, y;    // current level and position
  int ml, mx, my; // current max tiles in l, x, y

  SlideState *S; // slide with levels, dims, etc.
} ViewerState;

void viewerInit(ViewerState *V);
void viewerRefreshScreen(ViewerState *V);
void viewerHandleKeypress(ViewerState *V, int key);
void viewerProcessKeypress(ViewerState *V);
void viewerFree(ViewerState *V);
