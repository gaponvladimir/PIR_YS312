#ifndef SETTINGS_NOPRINTF_H_INCLUDED
#define SETTINGS_NOPRINTF_H_INCLUDED

//#define USE_NANOPRINTF

#ifdef USE_NANOPRINTF

#include "nanoprintf.h"

// redefine functions
#define cg_snprintf		npf_snprintf
#define cg_vsnprintf	npf_vsnprintf

#else

#include <stdio.h>
#include <stdlib.h>

// use standard functions
#define cg_snprintf		snprintf
#define cg_vsnprintf	vsnprintf

#endif

#endif

