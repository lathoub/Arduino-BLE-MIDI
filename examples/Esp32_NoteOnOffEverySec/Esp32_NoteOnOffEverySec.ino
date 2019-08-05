#include "BleMidi.h"

BLEMIDI_CREATE_INSTANCE(bm);

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // Serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  bm.begin("hehe");

  bm.onConnected(OnBleMidiConnected);
  bm.onDisconnected(OnBleMidiDisconnected);

  bm.setHandleNoteOn(OnBleMidiNoteOn);
  bm.setHandleNoteOff(OnBleMidiNoteOff);

  Serial.println(F("looping"));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  bm.sendNoteOn(60, 127, 1); // note 60, velocity 127 on channel 1
  bm.sendNoteOff(60, 127, 1);

  delay(1000);
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnBleMidiConnected() {
  Serial.println(F("Connected"));
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnBleMidiDisconnected() {
  Serial.println(F("Disconnected"));
}

// -----------------------------------------------------------------------------
// received note on
// -----------------------------------------------------------------------------
void OnBleMidiNoteOn(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOn from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}


// -----------------------------------------------------------------------------
// received note off
// -----------------------------------------------------------------------------
void OnBleMidiNoteOff(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOff from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}
