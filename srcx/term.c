#include "term.h"

struct termios orig_termios;

void slice(const char *str, char *result, size_t start, size_t end) {
  strncpy(result, str + start, end - start);
}

void clearScreen(void) {
  if (write(STDOUT_FILENO, "\x1b[2J", 4) < 0)
    die("clearScreen");
}

void clearText(void) {
  moveCursor(0, 0);
  if (write(STDOUT_FILENO, "\x1b[0J\x1b[1J", 8) < 0)
    die("clearText");
}

void hideCursor(void) {
  if (write(STDOUT_FILENO, "\x1b[?25l", 6) < 0)
    die("hideCursor");
}

void showCursor(void) {
  if (write(STDOUT_FILENO, "\x1b[?25h", 6) < 0)
    die("showCursor");
}

void moveCursor(int row, int col) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b[%d;%dH", row, col);
  if (write(STDOUT_FILENO, s, len) < 0)
    die("moveCursor");
}

void die(const char *s) {
  // clearScreen();
  // moveCursor(0, 0);

  perror(s);
  exit(1);
}

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");

  // show cursor
  if (write(STDOUT_FILENO, "\x1b[?25h", 6) < 0)
    die("disableRawMode");
}

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode); // Then is always called on exit
  struct termios raw = orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0; // NOTE: blocking, allows only single character input
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

int getWindowSize(int *rows, int *cols, int *vw, int *vh) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    die("ioctl(fd, TIOCGWINSZ, &ws) failed.");

  if (ws.ws_col == 0)
    die("Unsupported terminal.");

  *cols = ws.ws_col;
  *rows = ws.ws_row;
  *vw = ws.ws_xpixel;
  *vh = ws.ws_ypixel;
  return 0;
}

void getWindowSizeKitty(int *vw, int *vh) {
  // Read window size using kitty
  if (write(STDOUT_FILENO, "\x1b[14t", 5) < 0)
    die("getWindowSizeKitty");

  // Read response
  char str[16];
  int ch, n = 0;
  int p1 = -1;
  int p2 = -1;
  while (1) {
    ch = getKeypress();
    str[n] = ch;
    if (ch == 't') {
      break;
    }
    if (ch == ';') {
      p1 == -1 ? (p1 = n) : (p2 = n);
    }
    n++;
  }

  // Parse height and width
  char str_w[10];
  char str_h[10];
  slice(str, str_h, p1 + 1, p2);
  slice(str, str_w, p2 + 1, n);
  *vw = atoi(str_w);
  *vh = atoi(str_h);
}

int getKeypress(void) {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      }
    }

    return '\x1b';
  } else {
    return c;
  }
}
