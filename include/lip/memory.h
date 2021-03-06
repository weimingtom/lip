#ifndef LIP_MEMORY_H
#define LIP_MEMORY_H

/**
 * @defgroup memory Memory
 * @brief Memory managment functions
 *
 * @{
 */

#include "common.h"

#define lip_new(ALLOCATOR, TYPE) (TYPE*)(lip_malloc(ALLOCATOR, sizeof(TYPE)))
#define LIP_ARRAY_BLOCK(TYPE, LENGTH) \
	{ \
		.element_size = sizeof(TYPE), \
		.num_elements = LENGTH, \
		.alignment = LIP_ALIGN_OF(TYPE) \
	}
#define LIP_STATIC_ARRAY_LEN(ARRAY) (sizeof((ARRAY)) / sizeof((ARRAY)[0]))

typedef struct lip_memblock_info_s lip_memblock_info_t;

/// Allocator interface.
struct lip_allocator_s
{
	/// Allocation callback, should be similar to realloc.
	void*(*realloc)(lip_allocator_t* self, void* old, size_t size);
	/// Free callback, should be similar to free.
	void(*free)(lip_allocator_t* self, void* mem);
};

struct lip_memblock_info_s
{
	size_t element_size;
	size_t num_elements;
	uint8_t alignment;
	ptrdiff_t offset;
};

/// An allocator that uses `realloc` and `free`
LIP_API lip_allocator_t* const lip_default_allocator;

struct lip_max_align_helper
{
	long long d;
	void* e;
	float f;
	double g;
	void(*h)();
};

static const size_t LIP_MAX_ALIGNMENT = LIP_ALIGN_OF(struct lip_max_align_helper);

LIP_API lip_memblock_info_t
lip_align_memblocks(unsigned int num_blocks, lip_memblock_info_t** blocks);

LIP_MAYBE_UNUSED static inline void*
lip_locate_memblock(void* base, const lip_memblock_info_t* block)
{
	return (char*)base + block->offset;
}

/// Realloc using an allocator
LIP_MAYBE_UNUSED static inline void*
lip_realloc(lip_allocator_t* allocator, void* ptr, size_t size)
{
	return allocator->realloc(allocator, ptr, size);
}

/// Malloc using an allocator
LIP_MAYBE_UNUSED static inline void*
lip_malloc(lip_allocator_t* allocator, size_t size)
{
	return lip_realloc(allocator, 0, size);
}

/// Free using an allocator
LIP_MAYBE_UNUSED static inline void
lip_free(lip_allocator_t* allocator, void* ptr)
{
	allocator->free(allocator, ptr);
}

LIP_MAYBE_UNUSED static inline void*
lip_align_ptr(const void* ptr, size_t alignment)
{
	return (void*)(((uintptr_t)ptr + alignment - 1) / alignment * alignment);
}

/**
 * @}
 */

#endif
