#ifndef LIP_UNIVERSE_H
#define LIP_UNIVERSE_H

#include "common.h"
#include "runtime.h"
#include "lni.h"

typedef struct lip_universe_s lip_universe_t;

lip_universe_t*
lip_universe_create(lip_allocator_t* allocator);

void
lip_lms_destroy(lip_universe_t* universe);

void
lip_universe_link_function(lip_universe_t* universe, lip_function_t* function);

lip_lni_t*
lip_universe_lni(lip_universe_t* universe);

lip_allocator_t*
lip_universe_function_allocator(lip_universe_t* universe);

bool
lip_universe_find_symbol(
	lip_universe_t* universe,
	lip_string_ref_t symbol,
	lip_value_t* result
);

#endif
