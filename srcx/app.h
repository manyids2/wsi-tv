#include <stdint.h>

#include "cache.h"
#include "slide.h"
#include "view.h"

typedef struct App {
  // only render if dirty
  int dirty;

  // States
  Slide *S;
  View *V;
  Cache *C;
} App;
