CC = gcc
CFLAGS = -Wall -Wextra -g -O0

main: main.c allocator.c
	$(CC) $(CFLAGS) -o main main.c allocator.c

clean:
	rm -f main main.o allocator.o
