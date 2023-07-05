#include "buffer.h"
#include "term.h"
#include <assert.h>
#include <libpng/png.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
// https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/
const char b64chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
size_t b64_encoded_size(size_t inlen) {
  size_t ret;
  ret = inlen;
  if (inlen % 3 != 0)
    ret += 3 - (inlen % 3);
  ret /= 3;
  ret *= 4;
  return ret;
}

char *b64_encode(char *in, size_t len) {
  char *out;
  size_t elen;
  size_t i;
  size_t j;
  size_t v;

  if (in == NULL || len == 0)
    return NULL;

  elen = b64_encoded_size(len);
  out = malloc(elen + 1);
  out[elen] = '\0';

  // TODO: Read in 4 * 3 = 12 block size to rearrange v
  for (i = 0, j = 0; i < len; i += 3, j += 4) {
    v = in[i];
    v = i + 1 < len ? v << 8 | in[i + 1] : v << 8;
    v = i + 2 < len ? v << 8 | in[i + 2] : v << 8;

    out[j] = b64chars[(v >> 18) & 0x3F];
    out[j + 1] = b64chars[(v >> 12) & 0x3F];
    if (i + 1 < len) {
      out[j + 2] = b64chars[(v >> 6) & 0x3F];
    } else {
      out[j + 2] = '=';
    }
    if (i + 2 < len) {
      out[j + 3] = b64chars[v & 0x3F];
    } else {
      out[j + 3] = '=';
    }
  }

  return out;
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

void provisionImage(int index, int w, int h, uint32_t *buf) {
  char s[128]; // giri giri
  int len;

  // RGBA, each one byte ( char ) -> one uint32_t
  int inlen = w * h * sizeof(uint32_t);
  int inlen64 = b64_encoded_size(inlen);

  // Encode in base64
  char *buf64 = malloc(inlen64 * sizeof(char));
  buf64 = b64_encode((char *)buf, inlen);

  // Signal that we are sending first chunk, as RGBA(f=32), data(t=d), with
  // width and height so kitty can understand the image dims
  len = snprintf(s, sizeof(s), "\e_Gt=d,f=32,q=2,i=%d,s=%d,v=%d,m=%d;", index,
                 w, h, inlen64 > CHUNK);
  write(STDOUT_FILENO, s, len);

  // Loop over data
  int i = 0;
  while (inlen64) {
    // Remaining buffer
    write(STDOUT_FILENO, &buf64[CHUNK * i],
          MIN(inlen64, CHUNK)); // First the escape code, and not last chunk

    // Keep track of what we wrote
    inlen64 -= CHUNK;

    // Only if there is more for next iteration
    if (inlen64) {
      // End and escape code for next block
      len = snprintf(s, sizeof(s), "\e\\\e_Gm=%d;", inlen64 > CHUNK);
      write(STDOUT_FILENO, s, len);
    }

    // Advance buffer
    i++;
  }

  // Final end code
  len = snprintf(s, sizeof(s), "\e\\");
  write(STDOUT_FILENO, s, len);

  // Free resources
  free(buf64);
}

void displayImage(int index, int row, int col, int X, int Y, int Z) {
  char s[64]; // giri giri

  // Move cursor to origin
  moveCursor(row, col);

  // Tell kitty to display image that was provisioned
  int len = snprintf(s, sizeof(s), "\e_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\e\\",
                     index, X, Y, Z);
  write(STDOUT_FILENO, s, len);
}

void clearImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\e_Ga=d,d=i,i=%d;\e\\", index);
  write(STDOUT_FILENO, s, len);
}

void deleteImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\e_Ga=d,d=I,i=%d;\e\\", index);
  write(STDOUT_FILENO, s, len);
}
