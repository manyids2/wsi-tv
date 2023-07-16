#include "term.h"
#include <bits/stdint-uintn.h>

#define CHUNK 4096

void RGBAtoRGBbase64(size_t num_pixels, const uint32_t *buf, char *buf64);
void kittyProvisionImage(int index, int w, int h, char *buf64);
void kittyDisplayImage(int index, int row, int col, int X, int Y, int Z);
void kittyClearImage(int index);
void kittyDeleteImage(int index);
