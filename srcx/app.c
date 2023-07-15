#include "app.h"

void appInit(App *A, char *slide) {
  // Initialize slide
  slideInit(A->S, slide);
}

void appFree(App *A) { slideFree(A->S); }
