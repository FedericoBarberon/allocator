# Mini Memory Allocator

An educational `malloc`/`free`/`realloc` implementation written in C from scratch, built to understand how a memory allocator actually works under the hood: heap management, free lists, splitting, coalescing, and the low-level pitfalls that show up along the way.

This is **not** a production allocator — it's a learning project. The goal isn't to compete with `glibc`, but to understand the design decisions behind something that's normally used without a second thought.

## What it does

- `memalloc(size)` — reserves a block of memory of at least `size` bytes
- `memfree(ptr)` — frees a previously allocated block
- `memrealloc(ptr, size)` — resizes a previously allocated block, preserving its contents

Both functions operate on their own heap, requested from the OS once via `sbrk` on the first allocation.

## Internal design

**Explicit free list, sorted by memory address.** Every free block holds a header (size + magic number) and two pointers (`prev`/`next`) linking it to its free neighbors. Allocated blocks only carry the header — the free-list pointers are "recycled" as payload space while the block is in use.

**First-fit.** When looking for space for an allocation, the free list is walked and the first block that's big enough is used. It's not the most fragmentation-efficient strategy (best-fit, segregated lists, and buddy allocation all exist), but it's the simplest one to reason about correctly, which was the priority for this project.

**Splitting.** If the block found by first-fit is bigger than needed, it gets cut in two: the used portion goes to the caller, and the remainder becomes a new free node — but *only* if that remainder is big enough to hold a full `freelist_node` (header + 2 pointers). If it isn't, the whole block is handed to the caller unsplit, to avoid leaving an unusable gap in memory.

**Bidirectional coalescing.** When a block is freed, its physical neighbors (left and right) are checked for being free too, and if so they're merged into one larger block. This keeps memory from fragmenting into progressively smaller pieces with repeated malloc/free cycles.

**Realloc: shrink in place, grow by relocation.** `memrealloc` has two paths depending on the requested size:
- **Shrinking** (new size ≤ current size): the block is resized in place — no copy, no new allocation. The freed tail is turned into a new free node and coalesced with its neighbors, same as a partial free. If the leftover space isn't big enough to hold a valid `freelist_node`, the block is left as-is.
- **Growing** (new size > current size): a new block is requested via `memalloc`, the old contents are copied over with `memcpy`, and the old block is released via `memfree`. There's no attempt to grow in place by absorbing a contiguous free neighbor — this keeps the logic simple at the cost of some avoidable copies.

As with `memalloc`, a `size == 0` call to `memrealloc` behaves like `memfree`, and a `NULL` pointer behaves like `memalloc`.

**Magic numbers.** Every header carries a value (`BLOCK_MAGIC` / `FREE_MAGIC`) that lets common misuse be caught at runtime: double-free, and invalid pointers passed to `memfree`/`memrealloc`.

**Alignment.** All allocations are aligned to `alignof(max_align_t)`, same as standard `malloc`, so the returned pointer is valid for storing any C data type, including ones that require strict alignment.

## What it does NOT have (on purpose)

- Thread-safety — no locking of any kind, not safe to use from multiple threads
- More sophisticated fit strategies (best-fit, segregated free lists, buddy allocator)
- Returning memory to the OS (once requested via `sbrk`, the heap never shrinks)
- In-place growth for `memrealloc` (growing always relocates, even when a contiguous free block would fit)

These omissions are intentional: the focus of this project was understanding the fundamentals well (free lists, splitting, coalescing, alignment, resizing) before adding further complexity.
