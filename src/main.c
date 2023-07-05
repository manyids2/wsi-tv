#include "viewer.h"

// Global states
ViewerState V;
SlideState S;
BufferState B;

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: wsi-tv /path/to/slide \n");
    return 1;
  }

  // Give reference of buffer and slide to viewer
  V.S = &S;
  V.B = &B;

  // Get path to slide
  char *slide = argv[1];

  // Open slide, read slide props like level, dims
  // ( before viewerInit as we want to exit early if error )
  slideInit(V.S, slide);

  // Setup terminal ( disableRawMode called on exit )
  enableRawMode();

  // Start viewer
  viewerInit(&V);

  // Allocate buffers
  bufferInit(&B, V.mx, V.my, V.ts);

  // Main loop
  while (1) {
    viewerRefreshScreen(&V);   // Render output
    viewerProcessKeypress(&V); // Handle input
  }

  // Free all resources
  bufferFree(V.B);
  slideFree(V.S);
  viewerFree(&V);

  return 0;
}
