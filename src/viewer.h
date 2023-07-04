#include "buffer.h"
#include "term.h"
#include "slide.h"

typedef struct ViewerState {
  int dirty;      // only render if dirty
  int cols, rows; // term size

  int ww, wh; // world dims ( slide )
  int vw, vh; // view dims ( screen )
  int cw, ch; // cell dims
  
  SlideState* S; // slide with levels, dims, etc.
} ViewerState;

void viewerInit(ViewerState *V);
void viewerRefreshScreen(ViewerState *V);
void viewerHandleKeypress(ViewerState *V, int key);
void viewerProcessKeypress(ViewerState *V);
void viewerFree(ViewerState *V);
