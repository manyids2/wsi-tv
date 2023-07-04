wsi-tv: src/main.c
	$(CC) src/main.c src/term.c src/term.h src/buffer.c src/buffer.h src/slide.c src/slide.h src/viewer.c src/viewer.h -o wsi-tv -Wall -Wextra -pedantic -std=c99 -lopenslide

compile_commands.json: wsi-tv
	bear make wsi-tv
