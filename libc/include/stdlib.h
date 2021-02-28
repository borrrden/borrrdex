#pragma once

#include "__config.h"

__BEGIN_DECLS

#include "features.h"

#ifdef __cplusplus
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#include "bits/alltypes.h"

int atoi(const char *);
long atol(const char *);
long long atoll(const char *);

//_Noreturn void abort (void);

__END_DECLS