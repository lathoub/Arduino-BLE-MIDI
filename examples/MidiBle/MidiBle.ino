#include <BLEMIDI_Transport.h>

#include <hardware/BLEMIDI_ESP32_NimBLE.h>
//#include <hardware/BLEMIDI_ESP32.h>
//#include <hardware/BLEMIDI_nRF52.h>
//#include <hardware/BLEMIDI_ArduinoBLE.h>

BLEMIDI_CREATE_DEFAULT_INSTANCE()

unsigned long t0 = millis();
bool isConnected = false;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  MIDI.begin();

#ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  BLEMIDI.setHandleConnected(OnConnected);
  BLEMIDI.setHandleDisconnected(OnDisconnected);

  MIDI.setHandleNoteOn(OnNoteOn);
  MIDI.setHandleNoteOff(OnNoteOff);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  MIDI.read();

  if (isConnected && (millis() - t0) > 1000)
  {
    t0 = millis();

    MIDI.sendNoteOn (60, 100, 1); // note 60, velocity 127 on channel 1
  }
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
// Received note on
// -----------------------------------------------------------------------------
void OnNoteOn(byte channel, byte note, byte velocity) {
}

// -----------------------------------------------------------------------------
// Received note off
// -----------------------------------------------------------------------------
void OnNoteOff(byte channel, byte note, byte velocity) {
}
