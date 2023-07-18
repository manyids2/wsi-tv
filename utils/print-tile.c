#include <assert.h>
#include <openslide/openslide.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK 4096 // for kitty
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

void printB64EncodedImage(int w, int h, uint32_t *buf) {
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

  size_t sent_bytes = 0;
  while (sent_bytes < base64_size) {
    size_t chunk_size =
        base64_size - sent_bytes < CHUNK ? base64_size - sent_bytes : CHUNK;
    fwrite(base64_pixels + sent_bytes, chunk_size, 1, stdout);
    sent_bytes += chunk_size;
  }
  fflush(stdout);
  free(base64_pixels);
}

int main(int argc, char **argv) {
  if (argc != 7) {
    printf("Arguments: slide x y l w h\n");
    return 1;
  }
  const char *slide = argv[1];
  const int x = atoi(argv[2]);
  const int y = atoi(argv[3]);
  const int l = atoi(argv[4]);
  const int w = atoi(argv[5]);
  const int h = atoi(argv[6]);

  uint32_t *buf = malloc(w * h * 4);
  openslide_t *osr = openslide_open(slide);
  assert(osr != NULL && openslide_get_error(osr) == NULL);

  openslide_read_region(osr, buf, x, y, l, w, h);
  printB64EncodedImage(w, h, buf);

  openslide_close(osr);
  free(buf);
  return 0;
}
