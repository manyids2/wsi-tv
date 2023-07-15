#include <stdint.h>

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

  // Constants upto resize
  int64_t ww, wh; // in pixels
  int cols, rows; // term size
  int cw, ch;     // cell dims
  int ts;         // tile size

  // Main state ( which should change on keypress )
  int l, si, sj;    // current level and slide row, col
  int ol, osi, osj; // old level and slide row, col

  // Computed state
  int64_t wx, wy, sx, sy; // center positions in pixels
  int vmi, vmj, smi, smj; // view and slide maximums
} View;
