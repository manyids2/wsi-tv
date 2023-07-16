#include "app.h"

void appInit(App *A, char *slide) {
  // Initialize view, cache, slide
  viewInit(A->V, slide);
}

void appFree(App *A) { slideFree(A->V->S); }
