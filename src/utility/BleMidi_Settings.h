#pragma once

#include "BleMidi_Namespace.h"

//#define DEBUG
#define RELEASE

#if defined(RELEASE)
#define RELEASE_BUILD
#undef DEBUG_BUILD
#endif

#if defined(DEBUG)
#define DEBUG_BUILD
#undef RELEASE_BUILD
#endif


#if defined(RELEASE_BUILD)
#undef BLEMIDI_DEBUG
#undef BLEMIDI_DEBUG_VERBOSE
#endif

#if defined(DEBUG_BUILD)
#define BLEMIDI_DEBUG			   1
#undef BLEMIDI_DEBUG_VERBOSE
#define BLEMIDI_DEBUG_PARSING
#endif

