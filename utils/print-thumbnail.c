#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <openslide/openslide.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define CHUNK 4096 // for kitty
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define KITTY_ID 1

struct termios orig_termios;

static void die(const char *s) {
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
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0; // NOTE: blocking, allows only single character input
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

void move_cursor(int row, int col) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b[%d;%dH", row, col);
  write(STDOUT_FILENO, s, len);
}

void slice(const char *str, char *result, size_t start, size_t end) {
  strncpy(result, str + start, end - start);
}

int getKeypress(void) {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

static void get_window_size(int *rows, int *cols, int *vw, int *vh) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    die("ioctl(fd, TIOCGWINSZ, &ws) failed.");

  if (ws.ws_col == 0)
    die("Unsupported terminal.");

  *cols = ws.ws_col;
  *rows = ws.ws_row;
  *vw = ws.ws_xpixel;
  *vh = ws.ws_ypixel;
}

static void get_window_size_kitty(int *vw, int *vh) {
  // Read window size using kitty
  write(STDOUT_FILENO, "\x1b[14t", 5);

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

void set_cursor(int row, int col) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b[%d;%dH", row, col);
  write(STDOUT_FILENO, s, len);
}

static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(size_t num_pixels, const uint32_t *in, uint8_t *out) {
  // We receive RGBA pixel ( 32 bits )
  uint32_t pixel;
  uint8_t u1, u2, u3, u4;
  for (size_t i = 0; i < num_pixels; i++) {
    //      rgb -> encoded -> binary
    // 0x000000 -> AAAA    -> 000000 000000 000000 000000
    // 0xFF0000 -> /wAA    -> 111111 110000 000000 000000
    // 0x00FF00 -> AP8A    -> 000000 001111 111100 000000
    // 0x0000FF -> AAD/    -> 000000 000000 000011 111111

    // showing red
    pixel = (uint32_t)in[i]; // 32 bits, 4 bytes
    u1 = ((pixel & 0x00FC0000) >> 18) & 0x3F;
    u2 = ((pixel & 0x0003F000) >> 12) & 0x3F;
    u3 = ((pixel & 0x00000FC0) >> 6) & 0x3F;
    u4 = ((pixel & 0x0000003F) >> 0) & 0x3F;

    // Write it to out buffer
    out[i * 4] = base64enc_tab[u1];
    out[i * 4 + 1] = base64enc_tab[u2];
    out[i * 4 + 2] = base64enc_tab[u3];
    out[i * 4 + 3] = base64enc_tab[u4];
  }
  // null terminator
  out[num_pixels * 4] = 0;
}

static void provision_thumbnail(int w, int h, uint32_t *buf, uint8_t *buf64) {
  size_t num_pixels = w * h;
  size_t total_size = num_pixels * sizeof(uint32_t);
  size_t base64_size = total_size + 1;

  // Send RGB instead of RGBA
  base64_encode(num_pixels, buf, buf64);

  size_t sent_bytes = 0;
  while (sent_bytes < base64_size) {
    size_t chunk_size =
        base64_size - sent_bytes < CHUNK ? base64_size - sent_bytes : CHUNK;
    int cont = !!(sent_bytes + chunk_size < base64_size);
    if (sent_bytes == 0) {
      fprintf(stdout, "\x1B_Gt=d,f=24,q=2,i=%u,s=%d,v=%d,m=%d;", KITTY_ID, w, h,
              cont);
    } else {
      fprintf(stdout, "\x1B_Gm=%d;", cont);
    }
    fwrite(buf64 + sent_bytes, chunk_size, 1, stdout);
    fprintf(stdout, "\x1B\\");
    sent_bytes += chunk_size;
  }
  fflush(stdout);
}

static void print_thumbnail(int row, int col, int X, int Y) {
  char s[64];

  // Move cursor to row and col
  move_cursor(row, col);

  // Show the image
  int len =
      snprintf(s, sizeof(s), "\x1b_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\x1b\\",
               KITTY_ID, X, Y, 1);
  write(STDOUT_FILENO, s, len);
}

void delete_thumbnail(void) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=I,i=%d;\x1b\\", KITTY_ID);
  write(STDOUT_FILENO, s, len);
}

int get_thumbnail_dims(openslide_t *osr, int64_t *tw, int64_t *th) {
  // NOTE: Allocates memory for thumbnail

  // Get associated images
  int ret = -1;
  const char *const *associated_image_names =
      openslide_get_associated_image_names(osr);

  while (*associated_image_names) {
    int64_t w, h;
    const char *name = *associated_image_names;

    if (!strcmp(name, "thumbnail\0")) {
      // Get size of thumbnail
      openslide_get_associated_image_dimensions(osr, name, &w, &h);

      // Allocate
      *tw = w;
      *th = h;
      // Successful
      ret = 0;
    }

    associated_image_names++;
  }
  return ret;
}

static void init_term(int *ch, int *cw) {
  int rows, cols, vw, vh;

  // Read window size using IOCTL -> over ssh, vw, vh fail, so use kitty
  get_window_size(&rows, &cols, &vw, &vh);
  get_window_size_kitty(&vw, &vh);

  *cw = vw / cols;
  *ch = vh / rows;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: print-thumbnail slidepath\n");
    return 1;
  }
  const char *slide = argv[1];

  // Enable raw mode to read kitty response
  enableRawMode();

  // Get cell width and height for scaling
  int cw, ch;
  init_term(&cw, &ch);

  // read the slide
  openslide_t *osr = openslide_open(slide);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  // try to read thumbnail
  int64_t tw;
  int64_t th;
  int ret = get_thumbnail_dims(osr, &tw, &th);
  assert(ret == 0);

  // Allocate memory
  uint32_t *buf = malloc(th * tw * sizeof(uint32_t));

  // Read thumbnail
  openslide_read_associated_image(osr, "thumbnail", buf);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  // allocate for base64
  int64_t total_size = tw * th * sizeof(uint32_t);
  uint8_t *buf64 = (uint8_t *)malloc(total_size + 1);

  printf("%lu, %lu : %lu\n", tw, th, total_size);

  // send to kitty
  provision_thumbnail(tw, th, buf, buf64);

  // print
  print_thumbnail(0, 0, 0, 0);

  // wait to dismiss
  getKeypress();

  // free up on kitty
  delete_thumbnail();

  // free
  free(buf);
  free(buf64);
  openslide_close(osr);
  return 0;
}
