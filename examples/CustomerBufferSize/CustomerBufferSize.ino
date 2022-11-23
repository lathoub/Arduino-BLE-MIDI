#include <BLEMIDI_Transport.h>

 struct CustomBufferSizeSettings : public BLEMIDI_NAMESPACE::DefaultSettings {
  static const size_t MaxBufferSize = 16;
};

#include <hardware/BLEMIDI_ESP32_NimBLE.h>
//#include <hardware/BLEMIDI_ESP32.h>
//#include <hardware/BLEMIDI_ArduinoBLE.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

BLEMIDI_CREATE_CUSTOM_INSTANCE("Esp32-NimBLE-MIDI", MIDI, CustomBufferSizeSettings);

unsigned long t0 = millis();
bool isConnected = false;


// -----------------------------------------------------------------------------
// When BLE connected, LED will turn on (indication that connection was successful)
// When receiving a NoteOn, LED will go out, on NoteOff, light comes back on.
// This is an easy and conveniant way to show that the connection is alive and working.
// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("booting");

  MIDI.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  BLEMIDI.setHandleConnected([]() {
    isConnected = true;
    digitalWrite(LED_BUILTIN, HIGH);
  });

  BLEMIDI.setHandleDisconnected([]() {
    isConnected = false;
    digitalWrite(LED_BUILTIN, LOW);
  });

  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    digitalWrite(LED_BUILTIN, LOW);
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    digitalWrite(LED_BUILTIN, HIGH);
  });
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

    MIDI.sendNoteOn (60, 100, 1); // note 60, velocity 100 on channel 1
  }
}
