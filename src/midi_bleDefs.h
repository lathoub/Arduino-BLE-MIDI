#pragma once

#include "midi_bleNamespace.h"

#if ARDUINO
#include <Arduino.h>
#else
#include <inttypes.h>
typedef uint8_t byte;
#endif

BEGIN_BLEMIDI_NAMESPACE

#define BleBuffer_t Deque<byte, 44>

/*! \brief Create an instance of the library
 */
#define BLEMIDI_CREATE_INSTANCE(Type, Name)     \
    BLEMIDI_NAMESPACE::BleMidiTransport<Type> Name((Type&)SerialPort);


/*! \brief
*/
#define BLEMIDI_CREATE_DEFAULT_INSTANCE()       \
    BLEMIDI_CREATE_INSTANCE(bm);

END_BLEMIDI_NAMESPACE
