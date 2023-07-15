#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: app-shell args \n");
    return 1;
  }

  // Get arg
  char *arg = argv[1];
  printf("arg: %s\n", arg);

  // Exit
  exit(EXIT_SUCCESS);
  return 0;
}
