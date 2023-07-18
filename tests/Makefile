test_base64: Makefile test_base64.c
	$(CC) test_base64.c -o test_base64 -Wall -Wextra -pedantic -std=c99 -O3 -pg

test_threads: Makefile test_threads.c
	$(CC) test_threads.c -o test_threads -Wall -Wextra -pedantic -std=c99 -O3 -lpthread -lopenslide

example_pthreads: Makefile example_pthreads.c
	$(CC) example_pthreads.c -o example_pthreads -Wall -Wextra -pedantic -std=c99 -O3 -lpthread
