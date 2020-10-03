#pragma once

#include "BLEMIDI_Namespace.h"

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
