#include "buffer.h"
#include "slide.h"
#include "term.h"

#include <assert.h>

/*** state ***/
typedef struct TermState {
  int dirty;      // only render if dirty
  int cols, rows; // screen size

  int ww, wh; // world dims
  int vw, vh; // view dims
  int cw, ch; // cell dims

  // Slide properties
  int level_count;
  int64_t *level_w;
  int64_t *level_h;
  float *downsamples;
} TermState;

// Global state
TermState ts;

/*** output ***/
void viewerRefreshScreen(void) {
  if (ts.dirty == 0)
    return;

  // Initialize a buffer
  // realloc each time we append??
  // why not decent initial size and double like arrays??
  struct abuf ab = ABUF_INIT;

  // hide cursor
  abAppend(&ab, "\x1b[?25l", 6);

  // // write slidename
  // abAppend(&ab, "\x1b[?25l", 6);

  // // show cursor
  // abAppend(&ab, "\x1b[?25h", 6);

  // Finally write out
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

  // Reset dirty state
  ts.dirty = 0;
}

/*** control ***/
void viewerMoveCursor(int key) {
  switch (key) {
  case ARROW_LEFT:
  case ARROW_RIGHT:
  case ARROW_UP:
  case ARROW_DOWN:
    ts.dirty = 1;
    break;
  }
}

/*** input ***/
void viewerProcessKeypress(void) {
  int c = viewerReadKey();
  switch (c) {
  // Quit
  case 'q':
  case (CTRL_KEY('q')):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  // Cursor movement
  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    viewerMoveCursor(c);
    break;
  }
}

/*** init ***/
void initViewer(void) {
  // Clear screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // Init viewer state
  ts.dirty = 1;

  // Read window size
  if (getWindowSize(&ts.rows, &ts.cols, &ts.vw, &ts.vh) == -1)
    die("getWindowSize");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: wsi-tv slidepath\n");
    return 1;
  }

  // Open the slide
  char *slide = argv[1];
  openslide_t *osr = openslide_open(slide);

  // If we use die instead of assert, it prints to stdout :(
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  // Get count of levels in wsi pyramid
  ts.level_count = openslide_get_level_count(osr);

  // Allocate memory and store downsamples ( scale factors ), level_w, level_h
  ts.downsamples = malloc(ts.level_count * sizeof(float));
  ts.level_w = malloc(ts.level_count * sizeof(int64_t));
  ts.level_h = malloc(ts.level_count * sizeof(int64_t));
  for (int32_t level = 0; level < openslide_get_level_count(osr); level++) {
    ts.downsamples[level] = openslide_get_level_downsample(osr, level);
    openslide_get_level_dimensions(osr, level, &ts.level_w[level],
                                   &ts.level_h[level]);
  }

  // Start viewer
  enableRawMode();
  initViewer();

  while (1) {
    viewerRefreshScreen();
    viewerProcessKeypress();
  }

  // Close slide
  openslide_close(osr);

  // Free state data
  free(ts.downsamples);
  free(ts.level_w);
  free(ts.level_h);

  return 0;
}
