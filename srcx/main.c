#include "app.h"
#include <ev.h>

// Declare the app, slide, view
// TODO: Can we use static??
App A;
Slide S;
View V;
Cache C;

ev_io stdin_watcher;

static void stdin_cb(EV_P_ ev_io *w, int revents) {
  clearText();

  int c = getKeypress();
  switch (c) {

  // Quit
  case 'q':
  case (CTRL_KEY('q')):
    clearScreen();
    moveCursor(0, 0);
    ev_io_stop(EV_A_ w);         // Stop watching stdin
    ev_break(EV_A_ EVBREAK_ALL); // all nested ev_runs stop iterating
    break;

  // Up
  case ARROW_UP:
  case 'j':
    viewMoveDown(A.V);
    break;

  // Down
  case ARROW_DOWN:
  case 'k':
    viewMoveUp(A.V);
    break;

  // Left
  case ARROW_LEFT:
  case 'h':
    viewMoveLeft(A.V);
    break;

  // Right
  case ARROW_RIGHT:
  case 'l':
    viewMoveRight(A.V);
    break;

  // Zoom in
  case 'i':
    viewZoomIn(A.V);
    break;

  // Zoom out
  case 'o':
    viewZoomOut(A.V);
    break;

  // Toggle thumbnail
  case 't':
    break;

  // Debug info
  case 'd':
    A.V->debug = (A.V->debug + 1) % 3;
    break;
  }

  viewPrintDebug(A.V);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: wsi-tv path/to/slide \n");
    return 1;
  }

  // Get path to slide
  char *slide = argv[1];
  printf("slidepath: %s\n", slide);

  // Prep the terminal
  enableRawMode();
  hideCursor();

  // Associate with app and viewer
  V.S = &S;
  V.C = &C;
  A.V = &V;

  // Initialize
  appInit(&A, slide);

  // Start the event loop
  struct ev_loop *loop = EV_DEFAULT;

  // Watch stdin
  ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
  ev_io_start(loop, &stdin_watcher);

  // Now wait for events to arrive
  ev_run(loop, 0);

  // Free all resources
  appFree(&A);

  // // Reset terminal
  // clearScreen();
  // moveCursor(0, 0);

  // Exit successfully
  exit(EXIT_SUCCESS);
  return 0;
}
