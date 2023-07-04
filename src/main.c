#include "slide.h"
#include "viewer.h"

#include <assert.h>

/*** state ***/

// Global states
ViewerState V;
SlideState S;

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: wsi-tv slidepath\n");
    return 1;
  }

  // Open the slide, read slide props
  char *slide = argv[1];
  slideInit(&S, slide);

  // Setup terminal ( disableRawMode called on exit )
  enableRawMode();

  // Start viewer
  viewerInit(&V);

  // Main loop
  while (1) {
    viewerRefreshScreen(&V);   // Render output
    viewerProcessKeypress(&V); // Handle input
  }

  // Free all resources
  viewerFree(&V);
  slideFree(&S);

  return 0;
}
