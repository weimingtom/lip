#include <lip/lip.h>
#include <math.h>
#include <lip/bind.h>
#include "munit.h"
#include "test_helpers.h"

typedef struct lip_fixture_s lip_fixture_t;

struct lip_fixture_s
{
	lip_runtime_t* runtime;
	lip_context_t* context;
	lip_vm_t* vm;
};

static void*
setup(const MunitParameter params[], void* data)
{
	(void)params;
	(void)data;
	lip_fixture_t* fixture = lip_new(lip_default_allocator, lip_fixture_t);
	fixture->runtime = lip_create_runtime(lip_default_allocator);
	fixture->context = lip_create_context(fixture->runtime, NULL);
	lip_vm_config_t vm_config = {
		.os_len = 256,
		.cs_len = 256,
		.env_len = 256
	};
	fixture->vm = lip_create_vm(fixture->context, &vm_config);
	return fixture;
}

static void
teardown(void* fixture_)
{
	lip_fixture_t* fixture = fixture_;
	lip_destroy_runtime(fixture->runtime);
	lip_free(lip_default_allocator, fixture);
}

static
lip_function(lip_pow)
{
	lip_bind_args((number, x), (number, y));
	lip_return((lip_make_number(vm, pow(x, y))));
}

static MunitResult
direct(const MunitParameter params[], void* fixture_)
{
	(void)params;
	lip_fixture_t* fixture = fixture_;
	lip_vm_t* vm = fixture->vm;

	lip_value_t fn = lip_make_function(vm, lip_pow, 0, NULL);
	lip_value_t result;
	lip_exec_status_t status =
		lip_call(vm, &result, fn, 2, lip_make_number(vm, 3.5), lip_make_number(vm, 3.6));

	lip_assert_enum(lip_exec_status_t, LIP_EXEC_OK, ==, status);
	lip_assert_enum(lip_value_type_t, LIP_VAL_NUMBER, ==, result.type);
	munit_assert_double_equal(pow(3.5, 3.6), result.data.number, 3);

	return MUNIT_OK;
}

static lip_bind_wrap_function(pow, number, number, number)

static MunitResult
wrapper(const MunitParameter params[], void* fixture_)
{
	(void)params;
	lip_fixture_t* fixture = fixture_;
	lip_context_t* ctx = fixture->context;
	lip_vm_t* vm = fixture->vm;

	lip_value_t fn = lip_make_function(vm, lip_bind_wrapper(pow), 0, NULL);
	lip_value_t result;
	lip_exec_status_t status =
		lip_call(vm, &result, fn, 2, lip_make_number(vm, 3.5), lip_make_number(vm, 3.6));

	lip_assert_enum(lip_exec_status_t, LIP_EXEC_OK, ==, status);
	lip_assert_enum(lip_value_type_t, LIP_VAL_NUMBER, ==, result.type);
	munit_assert_double_equal(pow(3.5, 3.6), result.data.number, 3);

	status = lip_call(vm, &result, fn, 1, lip_make_number(vm, 3.5));
	lip_assert_enum(lip_exec_status_t, LIP_EXEC_ERROR, ==, status);
	lip_assert_str("Bad number of arguments (exactly 2 expected, got 1)", result);

	status = lip_call(vm, &result, fn, 2, lip_make_number(vm, 3.5), lip_make_string(vm, "wat"));
	lip_assert_enum(lip_exec_status_t, LIP_EXEC_ERROR, ==, status);
	lip_assert_str("Bad argument #2 (LIP_VAL_NUMBER expected, got LIP_VAL_STRING)", result);

	const lip_context_error_t* error = lip_traceback(ctx, vm, lip_make_nil(vm));
	unsigned int bottom = error->num_records - 1;
	lip_assert_string_ref_equal(lip_string_ref(__FILE__), error->records[bottom].filename);
	lip_assert_string_ref_equal(lip_string_ref(__func__), error->records[bottom].message);
	lip_assert_string_ref_equal(lip_string_ref(__FILE__), error->records[0].filename);
#define lip_stringify(x) lip_stringify1(x)
#define lip_stringify1(x) #x
	lip_assert_string_ref_equal(
		lip_string_ref(lip_stringify(lip_bind_wrapper(pow))),
		error->records[0].message
	);

	return MUNIT_OK;
}

static MunitTest tests[] = {
	{
		.name = "/direct",
		.test = direct,
		.setup = setup,
		.tear_down = teardown
	},
	{
		.name = "/wrapper",
		.test = wrapper,
		.setup = setup,
		.tear_down = teardown
	},
	{ .test = NULL }
};

MunitSuite bind = {
	.prefix = "/bind",
	.tests = tests
};
