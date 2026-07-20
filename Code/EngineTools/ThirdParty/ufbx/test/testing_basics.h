#ifndef UFBXT_TESTING_BASICS_INCLUDED
#define UFBXT_TESTING_BASICS_INCLUDED

#define ufbxt_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))

#if defined(_MSC_VER)
	#define ufbxt_noinline __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
	#define ufbxt_noinline __attribute__((noinline))
#else
	#define ufbxt_noinline
#endif

#endif
