#include "cache.h"
#include "slide.h"
#include "term.h"
#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

enum viewMoves { MOVE_LEFT, MOVE_RIGHT, MOVE_UP, MOVE_DOWN, ZOOM_IN, ZOOM_OUT };

typedef struct View {
  // wx, wy -> world level coords ( slide at max zoom )
  // l -> level (below are dependent on level)
  //
  //   View related
  //   vmi, vmj -> max tile coords
  //
  //   Slide related
  //   sx, sy -> coords
  //   si, sj -> tile coords ( row, col )
  //   smi, smj -> max tile coords
  int debug, dirty;

  // Always constant
  int ts;         // tile size
  int64_t ww, wh; // world width and height ( slide at max zoom )

  // Constants upto resize
  int vtw, vth;   // view terminal dims in pixels
  int vw, vh;     // view viewbox dims in pixels
  int cols, rows; // term size
  int cw, ch;     // cell dims
  int vx, vy;     // view center position
  int vmi, vmj;   // maximums
  int ox, oy;     // App level offsets, for sidebar

  // Constant for level
  int64_t sw, sh;   // slide dims at level
  float downsample; // Downsample level at scale
  int smi, smj;     // slide maximums

  // Main state ( which should change on keypress )
  int l, si, sj;          // current level and slide row, col at top left
  int ol, osi, osj;       // old level and slide row, col at top left
  int64_t wx, wy, sx, sy; // world and slide center positions in pixels

  Slide *S;
  Cache *C;
} View;

void viewInit(View *V, char *slide);
void viewSetLevel(View *V, int level);
void viewSetWorldPosition(View *V, int64_t wx, int64_t wy);

void viewMoveUp(View *V);
void viewMoveDown(View *V);
void viewMoveLeft(View *V);
void viewMoveRight(View *V);
void viewZoomIn(View *V);
void viewZoomOut(View *V);

void viewPrintDebug(View *V);
void viewFree(View *V);
