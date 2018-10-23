#pragma once

#include "BleMidi_Namespace.h"

#if ARDUINO
	#include <Arduino.h>
#else
	#include <inttypes.h>
	typedef uint8_t byte;
#endif

BEGIN_BLEMIDI_NAMESPACE

// -----------------------------------------------------------------------------

/*! \brief Create an instance of the library
 */
#define BLEMIDI_CREATE_INSTANCE(Name)                            \
    BLEMIDI_NAMESPACE::BleMidiInterface Name;


/*! \brief
*/
#define BLEMIDI_CREATE_DEFAULT_INSTANCE()                                      \
    BLEMIDI_CREATE_INSTANCE(bm);

END_BLEMIDI_NAMESPACE
