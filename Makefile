CC = gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -D__USE_MINGW_ANSI_STDIO=1
SOURCES = src/main.c src/library.c src/algorithms.c src/verification.c

all: libradaa libdaa.a
libradaa: $(SOURCES) src/libra.h
	$(CC) $(CFLAGS) $(SOURCES) -lm -o libradaa
algorithms.o: src/algorithms.c src/libra.h
	$(CC) $(CFLAGS) -c src/algorithms.c -o algorithms.o
libdaa.a: algorithms.o
	ar rcs libdaa.a algorithms.o
run: libradaa
	./libradaa
test: libradaa
	./libradaa --test
compare: libradaa
	./libradaa --compare
clean:
	rm -f libradaa libradaa.exe algorithms.o libdaa.a
.PHONY: all run test compare clean
