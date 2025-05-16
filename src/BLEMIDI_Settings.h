#pragma once

#include "BLEMIDI_Namespace.h"

BEGIN_BLEMIDI_NAMESPACE

//Common settings for all BLEMIDI classes
//This is the default settings for all BLEMIDI classes
struct _DefaultSettings : MIDI_NAMESPACE::DefaultSettings
{
    static const short MaxBufferSize = 64;
};

END_BLEMIDI_NAMESPACE
