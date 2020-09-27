#pragma once

#ifdef ARDUINO_ARCH_ESP32 
//#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <hardware/BLEMIDI_ESP32.h>
#endif

#ifdef AAA 
#include <hardware/BLEMIDI_nRF52.h>
#endif

#ifdef __AVR__ 
#include <hardware/BLEMIDI_ArduinoBLE.h>
#endif
