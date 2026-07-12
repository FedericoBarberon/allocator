#include "allocator.h"
#include <stdio.h>
#include <string.h>

int main() {
	size_t size = 12;
	int *arr = memalloc(size * sizeof(int));

	if (arr == NULL) {
		fprintf(stderr, "Failed to allocate arr");
		return 1;
	}

	printf("arr ptr: %p\n", (void*) arr);

	for (size_t i = 0; i < size; i++) {
		arr[i] = i+1;
	}

	for (size_t i = 0; i < size; i++) {
		printf("%d ", arr[i]);
	}

	printf("\n");

	memfree(arr);

	char *str = "Hello Heap!";
	char *heap_str = memalloc(strlen(str) + 1);

	if (heap_str == NULL) {
		fprintf(stderr, "Failed to allocate str");
		return 1;
	}

	printf("heap_str ptr: %p\n", (void*) heap_str); // Note that heap_str == arr because of the first-fit policy and the split_node impl

	strcpy(heap_str, str);

	printf("%s\n", heap_str);

	memfree(heap_str);
}
