#include <assert.h>
#include <openslide/openslide.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CHUNK 4096 // for kitty
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define KITTY_ID 1

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

static void print_thumbnail(int w, int h, uint32_t *buf, uint8_t *buf64) {
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

  // Display it
  char s[64];
  int len =
      snprintf(s, sizeof(s), "\x1b_Ga=p,i=%d,q=2,X=%d,Y=%d,C=1,z=%d;\x1b\\",
               KITTY_ID, 0, 0, 1);
  write(STDOUT_FILENO, s, len);

  // Compute number of lines to move
  for (size_t i = 0; i < 20; i++) {
    write(STDOUT_FILENO, "\n", 2);
  }
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

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: print-thumbnail slidepath\n");
    return 1;
  }
  const char *slide = argv[1];

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
  print_thumbnail(tw, th, buf, buf64);

  // free
  free(buf);
  free(buf64);
  openslide_close(osr);
  return 0;
}
