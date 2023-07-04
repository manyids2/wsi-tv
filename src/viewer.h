#include "buffer.h"
#include "term.h"

typedef struct ViewerState {
  int dirty;      // only render if dirty
  int cols, rows; // term size

  int ww, wh; // world dims ( slide )
  int vw, vh; // view dims ( screen )
  int cw, ch; // cell dims
} ViewerState;

void viewerInit(ViewerState *vs);
void viewerRefreshScreen(ViewerState *V);
void viewerHandleKeypress(ViewerState *V, int key);
void viewerProcessKeypress(ViewerState *V);
void viewerFree(ViewerState *V);
