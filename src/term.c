#include "term.h"

struct termios orig_termios;

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");

  // show cursor
  write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode); // Then is always called on exit
  struct termios raw = orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
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

void moveCursor(int row, int col) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b[%d;%dH", row, col);
  write(STDOUT_FILENO, s, len);
}

void hideCursor(void) { write(STDOUT_FILENO, "\x1b[?25l", 6); }
void showCursor(void) { write(STDOUT_FILENO, "\x1b[?25h", 6); }
void clearScreen(void) { write(STDOUT_FILENO, "\x1b[J", 3); }
