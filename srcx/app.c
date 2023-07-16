#include "app.h"

void appInit(App *A, char *slide) {
  // Initialize view, cache, slide
  viewInit(A->V, slide);
}

void appFree(App *A) {
  // Free cache, slide
  viewFree(A->V);
}
