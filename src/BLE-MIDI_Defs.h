#pragma once

#include "BLE-MIDI_Namespace.h"

// As specified in
// Specification for MIDI over Bluetooth Low Energy (BLE-MIDI)
// Version 1.0a, NOvember 1, 2015
// 3. BLE Service and Characteristics Definitions
#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

#if ARDUINO
#include <Arduino.h>
#else
#include <inttypes.h>
typedef uint8_t byte;
#endif

BEGIN_BLEMIDI_NAMESPACE

/*! \brief Create an instance of the library
 */
#define BLEMIDI_CREATE_INSTANCE(Type, DeviceName, Name)     \
typedef BLEMIDI_NAMESPACE::BLEMIDI<BLEMIDI_NAMESPACE::BLEMIDI_ESP32> BLEMIDI_t; \
BLEMIDI_t Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_t> MIDI((BLEMIDI_t &)Name);

/*! \brief Create an instance for ESP32 named <DeviceName>
*/
#define BLEMIDI_CREATE_ESP32_INSTANCE(DeviceName)       \
BLEMIDI_CREATE_INSTANCE(BLEMIDI_NAMESPACE::BLEMIDI_ESP32, DeviceName, bm);

/*! \brief Create a default instance for ESP32 named BLE-MIDI
*/
#define BLEMIDI_CREATE_DEFAULT_ESP32_INSTANCE()       \
BLEMIDI_CREATE_ESP32_INSTANCE("BLE-MIDI")

END_BLEMIDI_NAMESPACE
