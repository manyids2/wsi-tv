#include "term.h"
#include <bits/stdint-uintn.h>

#define CHUNK 4096

void RGBAtoRGBbase64(size_t total_pixels, const uint32_t *in, char *out);
void kittyProvisionImage(int index, int w, int h, uint32_t *buf,
                         uint8_t *buf64);
void kittyDisplayImage(int index, int row, int col, int X, int Y, int Z);
void kittyClearImage(int index);
void kittyDeleteImage(int index);
