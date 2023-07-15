#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "view.h"

typedef struct App {
  // only render if dirty
  int dirty;

  // States
  View *V;
} App;

void appInit(App *A, char *slide);
void appFree(App *A);
