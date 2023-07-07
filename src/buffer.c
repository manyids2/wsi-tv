#include "buffer.h"
#include "term.h"
#include <assert.h>
#include <libpng/png.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COMPRESSION_STRING ""

// --- buffer to make strings ---
void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); }

// --- buffer for tile images ---
void bufferInit(BufferState *B, int mx, int my, int ts) {
  B->mx = mx;
  B->my = my;
  B->ts = ts;

  // Allocate memory for pointers to buffers
  int max_buffers = mx * my;
  B->bufs = calloc(max_buffers, sizeof(int *));

  // Initialize values, memory at each tile position
  int index;
  for (int x = 0; x < mx; x++) {
    for (int y = 0; y < my; y++) {
      // Formula for index
      index = x * my + y;

      // Initialize ids of tiles at bufs[index]
      B->ll[index] = 0;
      B->xx[index] = 0;
      B->yy[index] = 0;

      // Allocate memory for one tile
      B->bufs[index] = malloc(B->ts * B->ts * 4 * sizeof(uint32_t));
    }
  }
}

void bufferFree(BufferState *B) {
  for (int i = 0; i < B->mx * B->my; i++) {
    free(&B->bufs[i]);
  }
}

// --- kitty related ---
static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(size_t in_len, const uint8_t *in, size_t out_len,
                         char *out) {
  size_t ii, io;
  uint_least32_t v;
  size_t rem;

  for (io = 0, ii = 0, v = 0, rem = 0; ii < in_len; ii++) {
    uint8_t ch;
    ch = in[ii];
    v = (v << 8) | ch;
    rem += 8;
    while (rem >= 6) {
      rem -= 6;
      if (io >= out_len)
        return -1; /* truncation is failure */
      out[io++] = base64enc_tab[(v >> rem) & 63];
    }
  }
  if (rem) {
    v <<= (6 - rem);
    if (io >= out_len)
      return -1; /* truncation is failure */
    out[io++] = base64enc_tab[v & 63];
  }
  while (io & 3) {
    if (io >= out_len)
      return -1; /* truncation is failure */
    out[io++] = '=';
  }
  if (io >= out_len)
    return -1; /* no room for null terminator */
  out[io] = 0;
  return io;
}

void encode_png(uint32_t *buffer, unsigned char *wbuffer, int w, int h) {
  // https://stackoverflow.com/questions/53237065/using-libpng-1-2-to-write-rgb-image-buffer-to-png-buffer-in-memory-causing-segme
  png_image wimage;
  memset(&wimage, 0, (sizeof wimage));
  wimage.version = PNG_IMAGE_VERSION;
  wimage.format = PNG_FORMAT_BGR;
  wimage.width = w;
  wimage.height = h;

  // My own shite
  int *nullptr = NULL;
  png_alloc_size_t wlength;

  // Get memory size
  bool wresult = png_image_write_to_memory(&wimage, nullptr, &wlength, 1,
                                           buffer, 0, nullptr);
  if (!wresult) {
    exit(1);
  }

  // Real write to memory
  wbuffer = (unsigned char *)malloc(wlength);
  wresult = png_image_write_to_memory(&wimage, wbuffer, &wlength, 1, buffer, 0,
                                      nullptr);
}

void provisionImage(int index, int w, int h, uint32_t *color_pixels) {
  int total_size = w * h * sizeof(uint32_t);
  size_t base64_size = ((total_size + 2) / 3) * 4;
  uint8_t *base64_pixels = (uint8_t *)malloc(base64_size + 1);

  /* base64 encode the data */
  int ret = base64_encode(total_size, (uint8_t *)color_pixels, base64_size + 1,
                          (char *)base64_pixels);
  if (ret < 0) {
    fprintf(stderr, "error: base64_encode failed: ret=%d\n", ret);
    exit(1);
  }

  /*
   * write kitty protocol RGBA image in chunks no greater than 4096 bytes
   *
   * <ESC>_Gf=32,s=<w>,v=<h>,m=1;<encoded pixel data first chunk><ESC>\
   * <ESC>_Gm=1;<encoded pixel data second chunk><ESC>\
   * <ESC>_Gm=0;<encoded pixel data last chunk><ESC>\
   */

  size_t sent_bytes = 0;
  while (sent_bytes < base64_size) {
    size_t chunk_size =
        base64_size - sent_bytes < CHUNK ? base64_size - sent_bytes : CHUNK;
    int cont = !!(sent_bytes + chunk_size < base64_size);
    if (sent_bytes == 0) {
      fprintf(stdout, "\x1B_Gt=d,f=32,q=2,i=%u,s=%d,v=%d,m=%d%s;", index, w, h,
              cont, COMPRESSION_STRING);
    } else {
      fprintf(stdout, "\x1B_Gm=%d;", cont);
    }
    fwrite(base64_pixels + sent_bytes, chunk_size, 1, stdout);
    fprintf(stdout, "\x1B\\");
    sent_bytes += chunk_size;
  }
  fflush(stdout);

  free(base64_pixels);
}

void displayImage(int index, int row, int col, int X, int Y, int Z) {
  char s[64]; // giri giri

  // Move cursor to origin
  moveCursor(row, col);

  // Tell kitty to display image that was provisioned
  int len =
      snprintf(s, sizeof(s), "\x1b_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\x1b\\",
               index, X, Y, Z);
  write(STDOUT_FILENO, s, len);
}

void clearImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=i,i=%d;\x1b\\", index);
  write(STDOUT_FILENO, s, len);
}

void deleteImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=I,i=%d;\x1b\\", index);
  write(STDOUT_FILENO, s, len);
}
