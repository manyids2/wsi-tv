wsi-tv: src/main.c src/term.c src/term.h src/buffer.c src/buffer.h src/slide.c src/slide.h src/viewer.c src/viewer.h
	$(CC) src/main.c src/term.c src/term.h src/buffer.c src/buffer.h src/slide.c src/slide.h src/viewer.c src/viewer.h -o wsi-tv -Wall -Wextra -pedantic -std=c99 -lopenslide -lm -lpng

print-tile: src/print-tile.c
	$(CC) src/print-tile.c -o print-tile -Wall -Wextra -pedantic -std=c99 -lopenslide
