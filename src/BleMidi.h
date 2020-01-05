/*!
 *  @file		BleMidi.h
 */

#pragma once

#include "utility/BleMidi_Settings.h"
#include "utility/BleMidi_Defs.h"

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

#if defined(ESP32)
#include "Ble_esp32.h"
#include "BleClient_esp32.h"
#endif
