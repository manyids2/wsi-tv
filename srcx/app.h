#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"
#include "slide.h"
#include "view.h"
#include "term.h"

typedef struct App {
  // only render if dirty
  int dirty;

  // States
  Slide *S;
  View *V;
  Cache *C;
} App;

void appInit(App *A, char *slide);
void appFree(App *A);
