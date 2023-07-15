#include "kitty.h"

// --- kitty related ---
// 26 + 26 + 10 + 2 = 64
static const uint8_t base64enc_tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void RGBAtoRGBbase64(size_t total_pixels, const uint32_t *in, char *out) {
  // We receive RGBA pixel ( 32 bits )
  uint32_t pixel;
  uint8_t u1, u2, u3, u4;
  for (size_t i = 0; i < total_pixels; i++) {
    pixel = (uint32_t)in[i]; // 32 bits, 4 bytes
    // c bit manipulation
    // 8 * 4 : xxxxxx xx.xxxx xxxx.xx xxxxxx xxxxxxxx ->
    // 6 * 4 : yyyyyy yyyyyy  yyyyyy  yyyyyy --------

    //      rgb -> encoded -> binary
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

void kittyProvisionImage(int index, int w, int h, uint32_t *buf,
                         uint8_t *buf64) {
  int total_size = w * h * sizeof(uint32_t);
  size_t base64_size = total_size;

  // Switch to RGB to get gains on base64 encoding
  // NOTE: Custom implementation
  RGBAtoRGBbase64(w * h, buf, (char *)buf64);

  /*
   * Switch to RGB to get gains on base64 encoding
   *
   * <ESC>_Gf=24,s=<w>,v=<h>,m=1;<encoded pixel data first chunk><ESC>\
   * <ESC>_Gm=1;<encoded pixel data second chunk><ESC>\
   * <ESC>_Gm=0;<encoded pixel data last chunk><ESC>\
   */

  size_t sent_bytes = 0;
  while (sent_bytes < base64_size) {
    size_t chunk_size =
        base64_size - sent_bytes < CHUNK ? base64_size - sent_bytes : CHUNK;
    int cont = !!(sent_bytes + chunk_size < base64_size);
    if (sent_bytes == 0) {
      fprintf(stdout, "\x1B_Gt=d,f=24,q=2,i=%u,s=%d,v=%d,m=%d%s;", index, w, h,
              cont, "");
    } else {
      fprintf(stdout, "\x1B_Gm=%d;", cont);
    }
    fwrite(buf64 + sent_bytes, chunk_size, 1, stdout);
    fprintf(stdout, "\x1B\\");
    sent_bytes += chunk_size;
  }
  fflush(stdout);
}

void kittyDisplayImage(int index, int row, int col, int X, int Y, int Z) {
  char s[64];

  // Move cursor to origin
  moveCursor(row, col);

  // Tell kitty to display image that was provisioned
  int len =
      snprintf(s, sizeof(s), "\x1b_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\x1b\\",
               index, X, Y, Z);
  if (write(STDOUT_FILENO, s, len) < 0)
    die("Display image write\n");
}

void kittyClearImage(int index) {
  char s[64];
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=i,i=%d;\x1b\\", index);
  if (write(STDOUT_FILENO, s, len) < 1)
    die("Clear image write\n");
}

void kittyDeleteImage(int index) {
  char s[64];
  int len = snprintf(s, sizeof(s), "\x1b_Ga=d,d=I,i=%d;\x1b\\", index);
  if (write(STDOUT_FILENO, s, len) < 0)
    die("Delete image write\n");
}
