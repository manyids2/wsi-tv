#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 26 + 26 + 10 + 2 = 64
static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(size_t in_len, const uint8_t *in, size_t out_len,
                         char *out) {
  size_t ii, io;
  uint32_t v;
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

int main(void) {
  int num_pixels = 1;

  printf("num_pixels : %d\n", num_pixels);

  int rgba_size = num_pixels * sizeof(uint32_t);
  uint32_t *rgba = calloc(num_pixels, sizeof(uint32_t));

  size_t rgba64_size = ((rgba_size + 2) / 3) * 4;
  uint8_t *rgba64 = (uint8_t *)malloc(rgba64_size + 1);

  int ret = base64_encode(rgba_size, (uint8_t *)rgba, rgba64_size + 1,
                          (char *)rgba64);

  if (ret < 0) {
    fprintf(stderr, "error: base64_encode failed: ret=%d\n", ret);
    exit(1);
  }

  printf("rgba (%03lu) : %s\n", strlen((char *)rgba64), rgba64);

  int rgb_size = num_pixels * 3 * sizeof(uint8_t);
  uint8_t *rgb = calloc(num_pixels * 3, sizeof(uint8_t));

  size_t rgb64_size = ((rgb_size + 2) / 3) * 4;
  uint8_t *rgb64 = (uint8_t *)malloc(rgb64_size + 1);

  ret = base64_encode(rgb_size, (uint8_t *)rgb, rgb64_size + 1, (char *)rgb64);

  if (ret < 0) {
    fprintf(stderr, "error: base64_encode failed: ret=%d\n", ret);
    exit(1);
  }

  printf("rgb  (%03lu) : %s\n", strlen((char *)rgb64), rgb64);

  free(rgba);
  free(rgba64);

  free(rgb);
  free(rgb64);
}
