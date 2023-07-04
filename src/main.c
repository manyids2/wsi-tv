#include "viewer.h"

// Global state
ViewerState V;

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: wsi-tv /path/to/slide \n");
    return 1;
  }

  // Get path to slide
  char *slide = argv[1];

  // Initialize slidestate and give reference to V
  SlideState S;
  V.S = &S;

  // Open slide, read slide props like level, dims
  slideInit(V.S, slide);

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
  slideFree(V.S);
  viewerFree(&V);

  return 0;
}
