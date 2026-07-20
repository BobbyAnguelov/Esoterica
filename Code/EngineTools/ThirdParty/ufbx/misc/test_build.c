
// Check everything under extra/ as well
#include "../extra/ufbx_libc.h"
#include "../extra/ufbx_math.h"
#include "../extra/ufbx_libc.c"
#include "../extra/ufbx_math.c"

#include "../ufbx.h"

#ifdef UFBXT_THREADS
	#define UFBX_OS_IMPLEMENTATION
	#include "../extra/ufbx_os.h"
#endif

#include "../ufbx.c"

int main(void)
{
	return 0;
}
