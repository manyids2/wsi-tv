#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void die(const char *s);

void disableRawMode();
void enableRawMode();

int viewerReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols, int *vw, int *vh);

/*** defines ***/
#define CTRL_KEY(k) ((k)&0x1f)
enum viewerKey { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
