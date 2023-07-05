#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k)&0x1f)
enum viewerKey { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };

void die(const char *s);

void disableRawMode(void);
void enableRawMode(void);

int getKeypress(void);
int getWindowSize(int *rows, int *cols, int *vw, int *vh);

void moveCursor(int row, int col);
void hideCursor(void);
void showCursor(void);
void clearScreen(void);
