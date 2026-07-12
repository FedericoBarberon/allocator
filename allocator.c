#include "allocator.h"
#include <assert.h>
#include<stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<stdalign.h>

#define HEAP_SIZE (4 * 1024) // 4KB
#define ALIGNMENT alignof(max_align_t)
#define BLOCK_MAGIC 0xDEADBEEF
#define FREE_MAGIC  0xBADC0FFE

typedef struct {
	size_t size;
	uint32_t magic;
} header_t;

typedef struct _freelist_node {
	header_t header;
	struct _freelist_node *prev;
	struct _freelist_node *next;
} freelist_node;

static uint8_t *heap = NULL;
static bool is_initialized = false;

// freelist invariant: Nodes are sorted by address in the memory pool
static freelist_node *head = NULL;

static inline size_t align_up(size_t n)
{
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}


static bool init_allocator() {
	heap = sbrk(HEAP_SIZE);

	if (heap == (void *)-1) {
		return false;
	}

	*(freelist_node *)heap = (freelist_node){
		.header = {.size = HEAP_SIZE - sizeof(header_t), .magic = FREE_MAGIC},
		.next = NULL,
		.prev = NULL
	};

	head = (freelist_node *)heap;
	is_initialized = true;
	return true;
}

// find a node from the freelist with enough size.
// first-fit policy
static freelist_node *find_fit(size_t size) {
	freelist_node *curr = head;

	while (curr && curr->header.size < size) curr = curr->next;

	if (!curr) {
		return NULL;
	}

	return curr;
}

// inserts node to the freelist maintaining the invariant
static void insert_node(freelist_node *node) {
	if (head == NULL) {
		head = node;
		node->prev = NULL;
		node->next = NULL;
		return;
	}

	if ((uint8_t*)head > (uint8_t*)node) {
		node->prev = NULL;
		node->next = head;
		head->prev = node;
		head = node;
		return;
	}

	freelist_node *curr = head;
	while (curr->next && (uint8_t*)curr->next < (uint8_t*)node) curr = curr->next;

	node->prev = curr;
	node->next = curr->next;

	curr->next = node;
	if (node->next) node->next->prev = node;
}

static void remove_node(freelist_node *node) {
	if (node->prev) node->prev->next = node->next;
	if (node->next) node->next->prev = node->prev;

	if (node == head) head = head->next;

	node->prev = NULL;
	node->next = NULL;
}

static inline size_t max(size_t a, size_t b) {
	return a > b ? a : b;
}

// Given a node and a requested size, computes how much space the allocation actually
// needs (allocated_block_size, aligned and no smaller than sizeof(freelist_node), so the
// block can safely become a freelist_node later if freed). If the remaining space after
// that allocation wouldn't be enough to hold a valid freelist_node, the whole node is
// handed to the user as-is and removed from the freelist. Otherwise, the node is shrunk
// to allocated_block_size and a new free node is created right after it with the
// remaining space.
static void split_node(freelist_node *node, size_t size) {
	size_t allocated_block_size = align_up(max(size + sizeof(header_t), sizeof(freelist_node)));

	if (node->header.size < allocated_block_size + sizeof(freelist_node) - sizeof(header_t)) {
		node->header.magic = BLOCK_MAGIC;
		remove_node(node);
		return;
	}

	size_t remaining_payload_size = node->header.size - allocated_block_size;

	freelist_node *new_node_ptr = (freelist_node *)((uint8_t *)node + allocated_block_size);

	if (node->prev) node->prev->next = new_node_ptr;
	if (node->next) node->next->prev = new_node_ptr;
	if (node == head) head = new_node_ptr;

	node->header.size = allocated_block_size - sizeof(header_t);
	node->header.magic = BLOCK_MAGIC;

	*new_node_ptr = (freelist_node){
		.header = {.size = remaining_payload_size, .magic = FREE_MAGIC},
		.prev = node->prev,
		.next = node->next
	};
}

static inline void *get_payload(freelist_node *node) {
	return (header_t *)node + 1;
}

static inline header_t *get_header(void *payload) {
	return (header_t *)payload - 1;
}

// returns true if node2 follows node1, considering the free space in between
static bool are_contiguous(freelist_node *node1, freelist_node *node2) {
	return (uint8_t*)get_payload(node1) + node1->header.size == (uint8_t*)node2;
}

// merges two contiguous nodes into one node, removing the second node and expanding the first one. Returns a ptr to
// the merged node, wich is always node1
// Precondition: node1 and node2 must be contiguous
static freelist_node *merge_nodes(freelist_node *node1, freelist_node *node2) {
	assert(are_contiguous(node1, node2));
	size_t new_size = node2->header.size + sizeof(header_t);

	remove_node(node2);
	node1->header.size += new_size;
	return node1;
}

static freelist_node *coalescing(freelist_node *node) {
	if (node->next && are_contiguous(node, node->next)) {
		merge_nodes(node, node->next);
	}

	if (node->prev && are_contiguous(node->prev, node)) {
		merge_nodes(node->prev, node);
		node = node->prev;
	}

	return node;
}

void *memalloc(size_t size) {
	if (size == 0) return NULL;

	if (!is_initialized && !init_allocator()) {
		return NULL;
	}

	size_t aligned_size = align_up(size);

	freelist_node *node = find_fit(aligned_size);

	if (!node) return NULL;

	split_node(node, aligned_size);
	return get_payload(node);
}

void memfree(void *ptr) {
	if (ptr == NULL) return;

	freelist_node *node = (freelist_node*)get_header(ptr);

	if ((uint8_t*)node < heap || (uint8_t*)node >= heap + HEAP_SIZE) {
		fprintf(stderr, "pointer outside heap\n");
		abort();
	}

	if (node->header.magic == FREE_MAGIC) {
		fprintf(stderr, "double free\n");
		abort();
	}

	if (node->header.magic != BLOCK_MAGIC) {
		fprintf(stderr, "invalid pointer\n");
		abort();
	}

	node->header.magic = FREE_MAGIC;
	insert_node(node);
	coalescing(node);
}
