#pragma once

#ifdef ARDUINO_ARCH_ESP32 
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
//#include <hardware/BLEMIDI_ESP32.h>
#endif

#include <hardware/BLEMIDI_nRF52.h>
#ifdef BLEMIDI_nRF52 
#endif


#ifdef ArduinoBLE 
#include <hardware/BLEMIDI_ArduinoBLE.h>
#endif
