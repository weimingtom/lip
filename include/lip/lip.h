#ifndef LIP_LIP_H
#define LIP_LIP_H

#include "common.h"
#include <stdarg.h>

#define LIP_USERDATA_SCOPE(F) \
	F(LIP_SCOPE_RUNTIME) \
	F(LIP_SCOPE_CONTEXT) \
	F(LIP_SCOPE_VM)

LIP_ENUM(lip_userdata_scope_t, LIP_USERDATA_SCOPE)

typedef struct lip_runtime_s lip_runtime_t;
typedef struct lip_runtime_config_s lip_runtime_config_t;
typedef struct lip_context_s lip_context_t;
typedef struct lip_script_s lip_script_t;
typedef struct lip_repl_handler_s lip_repl_handler_t;
typedef struct lip_context_error_s lip_context_error_t;
typedef struct lip_error_record_s lip_error_record_t;
typedef struct lip_ns_context_s lip_ns_context_t;
typedef void(*lip_panic_fn_t)(lip_context_t* ctx, const char* msg);

struct lip_repl_handler_s
{
	size_t(*read)(lip_repl_handler_t* self, void* buff, size_t size);
	void(*print)(lip_repl_handler_t* self, lip_exec_status_t status, lip_value_t result);
};

struct lip_error_record_s
{
	lip_string_ref_t filename;
	lip_loc_range_t location;
	lip_string_ref_t message;
};

struct lip_context_error_s
{
	lip_string_ref_t message;

	unsigned int num_records;
	lip_error_record_t* records;
};

LIP_API lip_runtime_t*
lip_create_runtime(lip_allocator_t* allocator);

LIP_API void
lip_destroy_runtime(lip_runtime_t* runtime);

LIP_API lip_context_t*
lip_create_context(lip_runtime_t* runtime, lip_allocator_t* allocator);

LIP_API void
lip_destroy_context(lip_runtime_t* runtime, lip_context_t* ctx);

LIP_API lip_panic_fn_t
lip_set_panic_handler(lip_context_t* ctx, lip_panic_fn_t panic_handler);

LIP_API const lip_context_error_t*
lip_get_error(lip_context_t* ctx);

LIP_API const lip_context_error_t*
lip_traceback(lip_context_t* ctx, lip_vm_t* vm, lip_value_t msg);

LIP_API lip_ns_context_t*
lip_begin_ns(lip_context_t* ctx, lip_string_ref_t name);

LIP_API void
lip_end_ns(lip_context_t* ctx, lip_ns_context_t* ns);

LIP_API void
lip_discard_ns(lip_context_t* ctx, lip_ns_context_t* ns);

LIP_API void
lip_declare_function(
	lip_ns_context_t* ns, lip_string_ref_t name, lip_native_fn_t fn
);

LIP_API lip_vm_t*
lip_create_vm(lip_context_t* ctx, lip_vm_config_t* config);

LIP_API void
lip_destroy_vm(lip_context_t* ctx, lip_vm_t* vm);

LIP_API bool
lip_lookup_symbol(lip_context_t* ctx, lip_string_ref_t symbol, lip_value_t* result);

LIP_API lip_script_t*
lip_load_script(lip_context_t* ctx, lip_string_ref_t filename, lip_in_t* input);

LIP_API void
lip_unload_script(lip_context_t* ctx, lip_script_t* script);

LIP_API lip_exec_status_t
lip_exec_script(lip_vm_t* vm, lip_script_t* script, lip_value_t* result);

LIP_API void
lip_repl(
	lip_vm_t* vm, lip_string_ref_t source_name, lip_repl_handler_t* repl_handler
);

void*
lip_get_userdata(lip_vm_t* vm, lip_userdata_scope_t scope, void* key);

void*
lip_set_userdata(lip_vm_t* vm, lip_userdata_scope_t scope, void* key, void* value);

LIP_API void
lip_load_builtins(lip_context_t* ctx);

LIP_API void
lip_reset_vm(lip_vm_t* vm);

LIP_API lip_vm_hook_t*
lip_set_vm_hook(lip_vm_t* vm, lip_vm_hook_t* hook);

LIP_API lip_exec_status_t
lip_call(
	lip_vm_t* vm,
	lip_value_t* result,
	lip_value_t fn,
	unsigned int num_args,
	...
);

LIP_API void
lip_set_native_location(lip_vm_t* vm, const char* file, int line);

LIP_API lip_value_t*
lip_get_args(const lip_vm_t* vm, uint8_t* num_args);

LIP_API lip_value_t*
lip_get_env(const lip_vm_t* vm, uint8_t* env_len);

LIP_MAYBE_UNUSED static inline lip_value_t
lip_make_boolean(lip_vm_t* vm, bool boolean)
{
	(void)vm;
	return (lip_value_t){
		.type = LIP_VAL_BOOLEAN,
		.data = { .boolean = boolean }
	};
}

LIP_MAYBE_UNUSED static inline lip_value_t
lip_make_nil(lip_vm_t* vm)
{
	(void)vm;
	return (lip_value_t){ .type = LIP_VAL_NIL };
}

LIP_MAYBE_UNUSED static inline lip_value_t
lip_make_number(lip_vm_t* vm, double number)
{
	(void)vm;
	return (lip_value_t){
		.type = LIP_VAL_NUMBER,
		.data = { .number = number }
	};
}

LIP_API lip_value_t
lip_make_string_copy(lip_vm_t* vm, lip_string_ref_t str);

LIP_API LIP_PRINTF_LIKE(2, 3) lip_value_t
lip_make_string(lip_vm_t* vm, const char* fmt, ...);

LIP_API lip_value_t
lip_make_stringv(lip_vm_t* vm, const char* fmt, va_list args);

LIP_API lip_value_t
lip_make_function(
	lip_vm_t* vm,
	lip_native_fn_t native_fn,
	uint8_t env_len,
	lip_value_t env[]
);

#define lip_call(vm, ...) \
	(lip_set_native_location(vm, __FILE__, __LINE__), lip_call(vm, __VA_ARGS__))

#endif
