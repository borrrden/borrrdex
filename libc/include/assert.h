#define _GNU_SOURCE
#include <sys/cdefs.h>

#undef assert
#undef _assert

#ifdef NDEBUG
#define assert ((void)0);
#define _assert ((void)0);
#else
#define _assert(e) assert(e)
#define assert(e) ((e) ? (void)0 : __assert(__func__, __FILE__, __LINE__, #e))
#endif

#if __ISO_C_VISIBLE >= 2011 && !defined(__cplusplus)
#define static_assert _Static_Assert
#endif

__BEGIN_DECLS
__attribute__((noreturn)) void __assert(const char*, const char*, int, const char*) ;
__END_DECLS