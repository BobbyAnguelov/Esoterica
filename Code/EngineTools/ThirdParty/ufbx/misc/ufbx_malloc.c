#ifndef UFBX_MALLOC_C_INCLUDED
#define UFBX_MALLOC_C_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

void ufbx_malloc_os_lock();
void ufbx_malloc_os_unlock();
void *ufbx_malloc_os_allocate(size_t size, size_t *p_allocated_size);
bool ufbx_malloc_os_free(void *pointer, size_t allocated_size);

#define UFBXI_MALLOC_FREE 0x1
#define UFBXI_MALLOC_USED 0x2
#define UFBXI_MALLOC_BLOCK 0x4

typedef struct ufbxi_malloc_node {
	struct ufbxi_malloc_node *prev_all, *next_all;
	struct ufbxi_malloc_node **prev_next_free, *next_free;
	size_t flags;
	size_t size;
} ufbxi_malloc_node;

#define UFBXI_MALLOC_NUM_SIZE_CLASSES 32

static ufbxi_malloc_node ufbxi_malloc_root;
static ufbxi_malloc_node *ufbxi_malloc_free_list[UFBXI_MALLOC_NUM_SIZE_CLASSES];

static size_t ufbxi_malloc_size_class(size_t size)
{
	size_t sc = 0;
	while (size > 16 && sc + 1 < UFBXI_MALLOC_NUM_SIZE_CLASSES) {
		sc++;
		size /= 2;
	}
	return sc;
}

static void ufbxi_malloc_link(ufbxi_malloc_node *prev, ufbxi_malloc_node *next)
{
	if (prev->next_all) prev->next_all->prev_all = next;
	next->next_all = prev->next_all;
	next->prev_all = prev;
	prev->next_all = next;
}

static void ufbxi_malloc_unlink(ufbxi_malloc_node *node)
{
	ufbxi_malloc_node *prev = node->prev_all, *next = node->next_all;
	if (next) next->prev_all = prev;
	prev->next_all = next;
	node->prev_all = node->next_all = NULL;
}

static void ufbxi_malloc_create(ufbxi_malloc_node *prev, ufbxi_malloc_node *node, size_t size, uint32_t flags)
{
	ufbxi_malloc_link(prev, node);
	node->size = size;
	node->flags = flags;
	node->prev_next_free = NULL;
	node->next_free = NULL;
}

static void ufbxi_malloc_link_free(ufbxi_malloc_node *node)
{
	size_t sc = ufbxi_malloc_size_class(node->size);
	ufbxi_malloc_node **p_free = &ufbxi_malloc_free_list[sc];
	ufbxi_malloc_node *free_node = *p_free;
	if (free_node) free_node->prev_next_free = &node->next_free;
	node->prev_next_free = p_free;
	node->next_free = *p_free;
	*p_free = node;
}

static void ufbxi_malloc_unlink_free(ufbxi_malloc_node *node)
{
	if (node->next_free) node->next_free->prev_next_free = node->prev_next_free;
	*node->prev_next_free = node->next_free;
	node->prev_next_free = NULL;
	node->next_free = NULL;
}

static bool ufbxi_malloc_block_end(ufbxi_malloc_node *node)
{
	if (!node) return true;
	if (!node->flags || node->flags == UFBXI_MALLOC_BLOCK) return true;
	return false;
}

void *ufbx_malloc(size_t size)
{
	if (size == 0) size = 1;
	size_t align = 2 * sizeof(void*);
	size = (size + align - 1) & ~(align - 1);
	ufbxi_malloc_node *node = NULL;

	ufbx_malloc_os_lock();

	size_t search_attempts = 16;
	size_t sc = ufbxi_malloc_size_class(size);
	for (; !node && sc < UFBXI_MALLOC_NUM_SIZE_CLASSES; sc++) {
		ufbxi_malloc_node *free_node = ufbxi_malloc_free_list[sc];
		if (!free_node) continue;

		for (size_t i = 0; i < search_attempts; i++) {
			if (!free_node) break;

			if (free_node->size >= size) {
				node = free_node;
				ufbxi_malloc_unlink_free(node);
				break;
			}

			free_node = free_node->next_free;
		}
	}

	if (node == NULL) {
		size_t allocated_size = 2 * sizeof(ufbxi_malloc_node) + size + align;
		void *memory = ufbx_malloc_os_allocate(allocated_size, &allocated_size);
		if (!memory) {
			ufbx_malloc_os_unlock();
			return NULL;
		}

		// Can't do anything if the memory is too small
		if (allocated_size <= 2 * sizeof(ufbxi_malloc_node) + align) {
			ufbx_malloc_os_free(memory, allocated_size);
			ufbx_malloc_os_unlock();
			return NULL;
		}

		size_t align_bytes = (align - (uintptr_t)memory % align) % align;
		char *aligned_memory = (char*)memory + align_bytes;
		size_t aligned_size = allocated_size - align_bytes;
		size_t free_space = aligned_size - sizeof(ufbxi_malloc_node) * 2;

		ufbxi_malloc_node *header = (ufbxi_malloc_node*)aligned_memory;
		ufbxi_malloc_node *block = header + 1;

		ufbxi_malloc_create(&ufbxi_malloc_root, header, allocated_size, UFBXI_MALLOC_BLOCK);
		header->next_free = memory;

		ufbxi_malloc_create(header, block, free_space, UFBXI_MALLOC_FREE);

		if (free_space >= size) {
			node = block;
		} else {
			ufbx_malloc_os_unlock();
			return NULL;
		}
	}

	if (node == NULL) {
		ufbx_malloc_os_unlock();
		return NULL;
	}

	char *data = (char*)(node + 1);

	size_t slack = node->size - size;
	if (slack >= 2 * sizeof(ufbxi_malloc_node)) {
		ufbxi_malloc_node *next = (ufbxi_malloc_node*)(data + size);
		ufbxi_malloc_create(node, next, slack - sizeof(ufbxi_malloc_node), UFBXI_MALLOC_FREE);
		ufbxi_malloc_link_free(next);
		node->size = size;
	}

	node->flags = UFBXI_MALLOC_USED;

	ufbx_malloc_os_unlock();
	return data;
}

void ufbx_free(void *ptr, size_t old_size)
{
	if (!ptr || old_size == 0) return;
	ufbx_malloc_os_lock();

	ufbxi_malloc_node *node = (ufbxi_malloc_node*)ptr - 1;
	node->flags = UFBXI_MALLOC_FREE;

	while (node->prev_all->flags == UFBXI_MALLOC_FREE) {
		node = node->prev_all;
		node->size += node->next_all->size + sizeof(ufbxi_malloc_node);
		ufbxi_malloc_unlink_free(node);
		ufbxi_malloc_unlink(node->next_all);
	}
	while (node->next_all && node->next_all->flags == UFBXI_MALLOC_FREE) {
		node->size += node->next_all->size + sizeof(ufbxi_malloc_node);
		ufbxi_malloc_unlink_free(node->next_all);
		ufbxi_malloc_unlink(node->next_all);
	}

	if (node->prev_all->flags == UFBXI_MALLOC_BLOCK && ufbxi_malloc_block_end(node->next_all)) {
		ufbxi_malloc_node *header = node->prev_all;
		ufbxi_malloc_unlink(header);
		ufbxi_malloc_unlink(node);
		if (!ufbx_malloc_os_free(header->next_free, header->size)) {
			ufbxi_malloc_link(&ufbxi_malloc_root, header);
			ufbxi_malloc_link(header, node);
		} else {
			node = NULL;
		}
	}

	if (node) {
		ufbxi_malloc_link_free(node);
	}

	ufbx_malloc_os_unlock();
}

void *ufbx_realloc(void *ptr, size_t old_size, size_t new_size)
{
	void *new_ptr = ufbx_malloc(new_size);
	if (!new_ptr) return NULL;

	// Don't expect memcpy()
	size_t copy_size = old_size < new_size ? old_size : new_size;
	char *dst = (char*)new_ptr;
	const char *src = (const char*)ptr;
	for (size_t i = 0; i < copy_size; i++) {
		dst[i] = src[i];
	}

	ufbx_free(ptr, old_size);
	return new_ptr;
}

#if defined(__cplusplus)
}
#endif

#endif

