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
void bufferInit(BufferState *B, int vtx, int vty, int ts) {
  B->vtx = vtx; // Constant as long as terminal is not resized
  B->vty = vty;
  B->ts = ts;

  // Compute buffer and base64 encoded size
  int total_size = B->ts * B->ts * sizeof(uint32_t);
  size_t base64_size = ((total_size + 2) / 3) * 4;

  // Allocate memory for pointers to buffers
  int max_buffers = vtx * vty;
  if (max_buffers > MAX_BUFFERS)
    die("Screen size exceeds expected bounds\r\n");

  // Put 0s in all buffers
  B->bufs = calloc(max_buffers, sizeof(int *));

  // Allocate memory for base64 encoded data
  B->buf64 = (uint8_t *)malloc(base64_size + 1);

  // Initialize values, memory at each tile position
  int index;
  for (int x = 0; x < vtx; x++) {
    for (int y = 0; y < vty; y++) {
      // Formula for index
      index = x * vty + y;

      // Initialize ids of tiles at bufs[index] to non existent value
      B->ll[index] = 0;
      B->wx[index] = 0;
      B->wy[index] = 0;
      B->owx[index] = 0;
      B->owy[index] = 0;

      // Initialize kitty ids of tiles
      B->ii[index] = index + 1;

      // Initialize position in grid
      B->vx[index] = x;
      B->vy[index] = y;

      // Allocate memory for one tile
      B->bufs[index] = malloc(B->ts * B->ts * sizeof(uint32_t));
    }
  }
}

void bufferFree(BufferState *B) {
  free(B->buf64);
  for (int i = 0; i < B->vtx * B->vty; i++) {
    free(&B->bufs[i]);
  }
  free(B->bufs);
}

// --- kitty related ---
static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int custom_base64_encode(size_t in_len, const uint8_t *in, size_t out_len,
                         char *out) {
  // We receive RGBA
  if (in_len % 4 != 0) {
    die("Unexpected in_len");
  }

  // Iterate and encode
  uint32_t pixel;
  uint8_t u1, u2, u3;
  int total_pixels = in_len / 4;
  for (int i = 0; i < total_pixels; i++) {
    pixel = in[i * 4]; // 32 bits
    // TODO: c bit manipulation
  }

  return 1;
}

int base64_encode(size_t in_len, const uint8_t *in, size_t out_len, char *out) {
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

void bufferProvisionImage(int index, int w, int h, uint32_t *buf,
                          uint8_t *buf64) {
  int total_size = w * h * sizeof(uint32_t);
  size_t base64_size = ((total_size + 2) / 3) * 4;

  /* base64 encode the data */
  int ret =
      base64_encode(total_size, (uint8_t *)buf, base64_size + 1, (char *)buf64);
  if (ret < 0) {
    fprintf(stderr, "error: base64_encode failed: ret=%d\n", ret);
    exit(1);
  }

  /*
   * Switch to RGB to get gains on base64 encoding
   *
   * <ESC>_Gf=24,s=<w>,v=<h>,m=1;<encoded pixel data first chunk><ESC>\
   * <ESC>_Gm=1;<encoded pixel data second chunk><ESC>\
   * <ESC>_Gm=0;<encoded pixel data last chunk><ESC>\
   */

  // TODO: make RGB instead of RGBA
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
    fwrite(buf64 + sent_bytes, chunk_size, 1, stdout);
    fprintf(stdout, "\x1B\\");
    sent_bytes += chunk_size;
  }
  fflush(stdout);
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
