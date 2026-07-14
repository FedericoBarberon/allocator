#include "allocator.h"
#include <assert.h>
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

	for (size_t i = 0; i < size; i++) arr[i] = i+1;

	for (size_t i = 0; i < size; i++) printf("%d ", arr[i]);

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

	int *arr2 = memalloc(3 * sizeof(int));

	if (arr2 == NULL) {
		fprintf(stderr, "Failed to allocate arr2");
		return 1;
	}

	for(size_t i = 0; i < 3; i++) arr2[i] = i+1;

	for(size_t i = 0; i < 3; i++) printf("%d ", arr2[i]); // 1 2 3
	printf("\n");

	arr2 = memrealloc(arr2, 5 * sizeof(int));

	if (arr2 == NULL) {
		fprintf(stderr, "Failed to reallocate arr2");
		return 1;
	}

	arr2[3] = 4; arr2[4] = 5;
	for(size_t i = 0; i < 5; i++) printf("%d ", arr2[i]); // 1 2 3 4 5
	printf("\n");

	int *arr3 = memrealloc(arr2, 2 * sizeof(int)); // Only shrinks the node, doesn't reallocate

	printf("arr2: %p\narr3: %p\n", (void*)arr2, (void*)arr3); // Both pointers are equal

	memfree(arr3);
}
