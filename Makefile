CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -fsanitize=address,undefined -fsanitize=alignment

main: main.c allocator.c
	$(CC) $(CFLAGS) -o main main.c allocator.c

clean:
	rm -f main main.o allocator.o
