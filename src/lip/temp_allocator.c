#include "ex/temp_allocator.h"
#include <stdlib.h>
#include "memory.h"
#include "utils.h"

static const size_t LIP_MAX_ALIGNMENT =
	LIP_ALIGN_OF(struct {
		char a;
		short b;
		int c;
		long long d;
		void* e;
		float f;
		double g;
	});

static void*
lip_alloc_from_chunk(lip_temp_chunk_t* chunk, size_t size)
{
	char* mem = lip_align_ptr(chunk->ptr, LIP_MAX_ALIGNMENT);
	if(mem + size <= chunk->end)
	{
		chunk->ptr = mem + size;
		return mem;
	}
	else
	{
		++chunk->num_failures;
		return NULL;
	}
}

static void*
lip_temp_allocator_small_alloc(lip_temp_allocator_t* allocator, size_t size)
{
	lip_temp_chunk_t** chunkp = &allocator->current_chunks;
	for(lip_temp_chunk_t* chunk = allocator->current_chunks;
		chunk != NULL;
		chunk = chunk->next
	)
	{
		chunkp = &chunk->next;

		void* mem = lip_alloc_from_chunk(chunk, size);

		if(
			(chunk->num_failures >= 3 || chunk->ptr >= chunk->end)
			&& chunk->next
		)
		{
			allocator->current_chunks = chunk->next;
		}

		if(mem) { return mem; }
	}

	lip_temp_chunk_t* new_chunk = lip_malloc(
		allocator->backing_allocator,
		sizeof(lip_temp_chunk_t) + allocator->chunk_size
	);
	if(!new_chunk) { return NULL; }

	new_chunk->num_failures = 0;
	new_chunk->ptr = new_chunk->start;
	new_chunk->end = new_chunk->start + allocator->chunk_size;
	new_chunk->next = *chunkp;
	*chunkp = new_chunk;

	if(!allocator->chunks) { allocator->chunks = new_chunk; }

	return lip_alloc_from_chunk(new_chunk, size);
}

static void*
lip_temp_allocator_large_alloc(lip_temp_allocator_t* allocator, size_t size)
{
	void* mem = lip_malloc(allocator->backing_allocator, size);
	if(!mem) { return NULL; }

	lip_large_alloc_t* large_alloc =
		lip_temp_allocator_small_alloc(allocator, sizeof(lip_large_alloc_t));
	if(!large_alloc)
	{
		lip_free(allocator->backing_allocator, mem);
		return NULL;
	}

	large_alloc->ptr = mem;
	large_alloc->next = allocator->large_allocs;
	allocator->large_allocs = large_alloc;

	return mem;
}

static void*
lip_temp_allocator_realloc(lip_allocator_t* self, void* old, size_t size)
{
	// This allocator cannot do reallocation
	if(old) { return NULL; }

	lip_temp_allocator_t* temp_allocator = (lip_temp_allocator_t*)self;

	if(size > temp_allocator->chunk_size)
	{
		return lip_temp_allocator_large_alloc(temp_allocator, size);
	}
	else
	{
		return  lip_temp_allocator_small_alloc(temp_allocator, size);
	}
}

static void
lip_temp_allocator_free(lip_allocator_t* self, void* ptr)
{
	(void)self;
	(void)ptr;
}

lip_allocator_t*
lip_temp_allocator_create(lip_allocator_t* allocator, size_t chunk_size)
{
	lip_temp_allocator_t* temp_allocator =
		lip_new(allocator, lip_temp_allocator_t);
	temp_allocator->vtable.realloc = lip_temp_allocator_realloc;
	temp_allocator->vtable.free = lip_temp_allocator_free;
	temp_allocator->backing_allocator = allocator;
	temp_allocator->current_chunks = NULL;
	temp_allocator->chunks = NULL;
	temp_allocator->large_allocs = NULL;
	temp_allocator->chunk_size = LIP_MAX(chunk_size, sizeof(lip_large_alloc_t));
	return &temp_allocator->vtable;
}

void
lip_temp_allocator_destroy(lip_allocator_t* allocator)
{
	lip_temp_allocator_reset(allocator);

	lip_temp_allocator_t* temp_allocator = (lip_temp_allocator_t*)allocator;
	for(
		lip_temp_chunk_t* chunk = temp_allocator->chunks;
		chunk != NULL;
	)
	{
		lip_temp_chunk_t* next_chunk = chunk->next;
		lip_free(temp_allocator->backing_allocator, chunk);
		chunk = next_chunk;
	}

	lip_free(temp_allocator->backing_allocator, temp_allocator);
}

void
lip_temp_allocator_reset(lip_allocator_t* allocator)
{
	lip_temp_allocator_t* temp_allocator = (lip_temp_allocator_t*)allocator;

	for(
		lip_large_alloc_t* large_alloc = temp_allocator->large_allocs;
		large_alloc != NULL;
		large_alloc = large_alloc->next
	)
	{
		lip_free(temp_allocator->backing_allocator, large_alloc->ptr);
	}

	temp_allocator->large_allocs = NULL;

	for(
		lip_temp_chunk_t* chunk = temp_allocator->chunks;
		chunk != NULL;
		chunk = chunk->next
	)
	{
		chunk->num_failures = 0;
		chunk->ptr = chunk->start;
	}

	temp_allocator->current_chunks = temp_allocator->chunks;
}