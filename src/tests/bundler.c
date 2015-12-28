#include <stdlib.h>
#include "module.h"

int main(int argc, char* argv[])
{
	lip_asm_t lasm;
	lip_asm_init(&lasm, lip_default_allocator);

	lip_function_t* function = load_asm(&lasm);
	function->arity = 0;

	lip_bundler_t bundler;
	lip_bundler_init(&bundler, lip_default_allocator);

	lip_module_t* module = bundle_functions(&bundler, function);
	lip_module_print(module);

	lip_free(lip_default_allocator, module);
	lip_bundler_cleanup(&bundler);

	lip_free(lip_default_allocator, function);
	lip_asm_cleanup(&lasm);

	return EXIT_SUCCESS;
}