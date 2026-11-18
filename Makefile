CC = gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -D__USE_MINGW_ANSI_STDIO=1
SOURCES = src/main.c src/library.c src/algorithms.c src/workbench.c src/evidence.c src/tests.c
HEADERS = src/library.h src/algorithms.h src/workbench.h src/counters.h
all: libradaa
libradaa: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -lm -o libradaa
run: libradaa
	./libradaa
test: libradaa
	./libradaa --test
bench: libradaa
	./libradaa --bench
clean:
	rm -f libradaa libradaa.exe
.PHONY: all run test bench clean
