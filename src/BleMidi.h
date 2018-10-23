/*!
 *  @file		BleMidi.h
 */

#pragma once

#include "utility/BleMidi_Defs.h"

BEGIN_BLEMIDI_NAMESPACE

/*! \brief The main class for AppleMidiInterface handling.\n
	See member descriptions to know how to use it,
	or check out the examples supplied with the library.
 */
class BleMidiInterface
{
public:
	// Constructor and Destructor
	inline  BleMidiInterface();
	inline ~BleMidiInterface();
};

END_BLEMIDI_NAMESPACE

// -----------------------------------------------------------------------------

#include "BleMidi.hpp"
