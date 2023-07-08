#include "buffer.h"
#include "term.h"

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
  if (max_buffers > MAX_BUFFERS)
    die("Screen size exceeds expected bounds\r\n");

  // Put 0s in all buffers
  B->bufs = calloc(max_buffers, sizeof(int *));

  // Initialize values, memory at each tile position
  int index;
  for (int x = 0; x < mx; x++) {
    for (int y = 0; y < my; y++) {
      // Formula for index
      index = x * my + y;

      // Initialize ids of tiles at bufs[index] to non existent value
      B->ll[index] = 0;
      B->xx[index] = 0;
      B->yy[index] = 0;

      // Allocate memory for one tile
      B->bufs[index] = malloc(B->ts * B->ts * sizeof(uint32_t));
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

void bufferLoadImage(openslide_t *osr, int l, int tx, int ty, int ts,
                     float downsample, uint32_t *buf) {
  int sx = tx * ts * downsample;
  int sy = ty * ts * downsample;
  openslide_read_region(osr, buf, sx, sy, l, ts, ts);
  assert(openslide_get_error(osr) == NULL);
}

void bufferProvisionImage(int index, int w, int h, uint32_t *buf) {
  int total_size = w * h * sizeof(uint32_t);
  size_t base64_size = ((total_size + 2) / 3) * 4;
  uint8_t *base64_pixels = (uint8_t *)malloc(base64_size + 1);

  /* base64 encode the data */
  int ret = base64_encode(total_size, (uint8_t *)buf, base64_size + 1,
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

void bufferDisplayImage(int index, int row, int col, int X, int Y, int Z) {
  char s[64]; // giri giri

  // Move cursor to origin
  moveCursor(row, col);

  // Tell kitty to display image that was provisioned
  int len =
      snprintf(s, sizeof(s), "\x1b_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\x1b\\",
               index, X, Y, Z);
  write(STDOUT_FILENO, s, len);
}

void bufferClearImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=i,i=%d;\x1b\\", index);
  write(STDOUT_FILENO, s, len);
}

void bufferDeleteImage(int index) {
  char s[32]; // giri giri
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=I,i=%d;\x1b\\", index);
  write(STDOUT_FILENO, s, len);
}
