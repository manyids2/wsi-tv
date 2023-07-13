#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int num_pixels = 1024 * 1024 * 32;
size_t num_reps = 32;

// definitions
void base64_encode(size_t in_len, const uint8_t *in, size_t out_len, char *out);
void custom_base64_encode(size_t total_pixels, const uint32_t *in, char *out);

// macros
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
  ((byte)&0x80 ? '1' : '0'), ((byte)&0x40 ? '1' : '0'),                        \
      ((byte)&0x20 ? '1' : '0'), ((byte)&0x10 ? '1' : '0'),                    \
      ((byte)&0x08 ? '1' : '0'), ((byte)&0x04 ? '1' : '0'),                    \
      ((byte)&0x02 ? '1' : '0'), ((byte)&0x01 ? '1' : '0')

// printf("Leading text "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(byte));
// For multi-byte types
// printf("m: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n",
//   BYTE_TO_BINARY(m>>8), BYTE_TO_BINARY(m));

// base64 encoding table ( 26 + 26 + 10 + 2 = 64 )
static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// from glkitty
void base64_encode(size_t in_len, const uint8_t *in, size_t out_len,
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
        return; /* truncation is failure */
      out[io++] = base64enc_tab[(v >> rem) & 63];
    }
  }
  if (rem) {
    v <<= (6 - rem);
    if (io >= out_len)
      return; /* truncation is failure */
    out[io++] = base64enc_tab[v & 63];
  }
  while (io & 3) {
    if (io >= out_len)
      return; /* truncation is failure */
    out[io++] = '=';
  }
  if (io >= out_len)
    return; /* no room for null terminator */
  out[io] = 0;
  return;
}

// our implementation
void custom_base64_encode(size_t total_pixels, const uint32_t *in, char *out) {
  // We receive RGBA pixel ( 32 bits )
  uint32_t pixel;
  uint8_t u1, u2, u3, u4;
  for (size_t i = 0; i < total_pixels; i++) {
    pixel = (uint32_t)in[i]; // 32 bits, 4 bytes
    // c bit manipulation
    //            R          G        B         A
    // 8 * 4 : xxxxxx xx.xxxx xxxx.xx xxxxxx xxxxxxxx ->
    // 6 * 4 : yyyyyy yyyyyy  yyyyyy  yyyyyy --------

    //      RGB -> encoded -> binary
    // 0x000000 -> AAAA    -> 000000 000000 000000 000000
    // 0xFF0000 -> /wAA    -> 111111 110000 000000 000000
    // 0x00FF00 -> AP8A    -> 000000 001111 111100 000000
    // 0x0000FF -> AAD/    -> 000000 000000 000011 111111

    // showing red
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
  out[total_pixels * 4] = 0;
}

int main(void) {

  printf("num_pixels : %d\n", num_pixels);

  // ----------- inits ------------------
  // malloc, so random bit values,
  // optimizer should not do anything

  // -- rgba --
  int rgba_size = num_pixels * sizeof(uint32_t);
  size_t rgba64_size = ((rgba_size + 2) / 3) * 4;
  uint32_t *rgba = malloc(num_pixels * sizeof(uint32_t));
  uint8_t *rgba64 = (uint8_t *)malloc(rgba64_size + 1);

  // -- rgb --
  int rgb_size = num_pixels * 3 * sizeof(uint8_t);
  size_t rgb64_size = ((rgb_size + 2) / 3) * 4;
  uint8_t *rgb = malloc(num_pixels * 3 * sizeof(uint8_t));
  uint8_t *rgb64 = (uint8_t *)malloc(rgb64_size + 1);

  // -- crgb --
  int crgb64_size = num_pixels * sizeof(uint32_t);
  uint8_t *crgb64 = (uint8_t *)malloc(crgb64_size + 1);

  // ----------- RGBA base64_encode ------------------

  // for (size_t i = 0; i < num_reps; i++) {
  //   base64_encode(rgba_size, (uint8_t *)rgba, rgba64_size + 1, (char
  //   *)rgba64); printf("%05lu: rgba (%03lu)\n", i, strlen((char *)rgba64));
  // }

  // ----------- RGB base64_encode ------------------

  for (size_t i = 0; i < num_reps; i++) {
    base64_encode(rgb_size, (uint8_t *)rgb, rgb64_size + 1, (char *)rgb64);
    printf("%05lu: rgb  (%03lu)\n", i, strlen((char *)rgb64));
  }

  // ----------- custom RGB base64_encode ------------------

  for (size_t i = 0; i < num_reps * 32; i++) {
    custom_base64_encode(num_pixels, rgba, (char *)crgb64);
    printf("%05lu: crgb (%03lu)\n", i, strlen((char *)crgb64));
  }

  // ----------- free ------------------

  free(rgba);
  free(rgba64);

  free(rgb);
  free(rgb64);
  free(crgb64);
}
