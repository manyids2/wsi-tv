#include "app.h"
#include <ev.h>

ev_io stdin_watcher;
static void stdin_cb(EV_P_ ev_io *w, int revents) {
  printf("stdin ready (%d)\r\n", revents);

  // HandleKeypress code goes here...
  //
  // ...
  //
  //

  // Stop watching stdin
  ev_io_stop(EV_A_ w);

  // this causes all nested ev_run's to stop iterating
  ev_break(EV_A_ EVBREAK_ALL);
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

  // Declare the app, slide, view
  // TODO: Can we use static??
  static App A;
  static Slide S;
  static View V;
  static Cache C;

  // Associate with app
  A.S = &S;
  A.V = &V;
  A.C = &C;

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

  // Reset terminal
  clearScreen();
  moveCursor(0, 0);

  // Exit successfully
  exit(EXIT_SUCCESS);
  return 0;
}
