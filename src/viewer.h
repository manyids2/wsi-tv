#include "buffer.h"
#include "slide.h"
#include "term.h"
#include <math.h>

typedef struct ViewerState {
  int dirty; // only render if dirty

  int cols, rows; // term size
  int ww, wh;     // world dims ( slide )
  int vw, vh;     // view dims ( screen )
  int cw, ch;     // cell dims

  int l, x, y;    // current level and position
  int ml, mx, my; // current max tiles in l, x, y
  int ts;         // tile size

  SlideState *S;  // slide with levels, dims, etc.
  BufferState *B; // struct with current buffers
} ViewerState;

void viewerInit(ViewerState *V);
void viewerFree(ViewerState *V);

void viewerRefreshScreen(ViewerState *V);
void viewerHandleKeypress(ViewerState *V, int key);
void viewerProcessKeypress(ViewerState *V);

void viewerAllocateThumbnail(ViewerState *V);
