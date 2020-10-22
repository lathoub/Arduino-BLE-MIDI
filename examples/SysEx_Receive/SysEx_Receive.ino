#include <BLEMIDI_Transport.h>

//#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <hardware/BLEMIDI_ESP32.h>
//#include <hardware/BLEMIDI_nRF52.h>
//#include <hardware/BLEMIDI_ArduinoBLE.h>

BLEMIDI_CREATE_INSTANCE("CustomName", MIDI)

bool isConnected = false;

//#define LOGGING

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  while (!Serial);

#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  BLEMIDI.setHandleConnected(OnConnected);
  BLEMIDI.setHandleDisconnected(OnDisconnected);
 
  MIDI.setHandleSystemExclusive(OnMidiSysEx);

  // Listen for MIDI messages on channel 1
  MIDI.begin();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  // Listen to incoming notes
  MIDI.read();
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// Device connected
// -----------------------------------------------------------------------------
void OnConnected() {
  isConnected = true;
#ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, HIGH);
#endif
}

// -----------------------------------------------------------------------------
// Device disconnected
// -----------------------------------------------------------------------------
void OnDisconnected() {
  isConnected = false;
#ifdef LED_BUILTIN
  digitalWrite(LED_BUILTIN, LOW);
#endif
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnMidiSysEx(byte* data, unsigned length) {
  Serial.print(F("SYSEX: ("));
  Serial.print(length);
  Serial.print(F(" bytes) "));
  for (uint16_t i = 0; i < length; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
