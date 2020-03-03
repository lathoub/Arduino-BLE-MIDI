#define DEBUG 4
#include <midi_bleTransport.h>
#include <Ble_esp32.h>


typedef BLEMIDI_NAMESPACE::BleMidiTransport<BLEMIDI_NAMESPACE::BluetoothEsp32> bleMIDI_t;
bleMIDI_t bm("Huzzah BLE MIDI");
MIDI_NAMESPACE::MidiInterface<bleMIDI_t> MIDI((bleMIDI_t &)bm);

USING_NAMESPACE_BLEMIDI

unsigned long t0 = millis();
bool isConnected = false;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // Serial communications and wait for port to open:
  DEBUG_BEGIN(115200);

  MIDI.begin(1);

  bm.onConnected(OnBleMidiConnected);
  bm.onDisconnected(OnBleMidiDisconnected);

  MIDI.setHandleNoteOn(OnBleMidiNoteOn);
  MIDI.setHandleNoteOff(OnBleMidiNoteOff);

  N_DEBUG_PRINTLN(F("Ready"));
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

    MIDI.sendNoteOn(60, 127, 1); // note 60, velocity 127 on channel 1
    MIDI.sendNoteOff(60, 0, 1);
  }

}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnBleMidiConnected() {
  N_DEBUG_PRINTLN(F("Connected"));
  isConnected = true;
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnBleMidiDisconnected() {
  Serial.println(F("Disconnected"));
  isConnected = false;
}

// -----------------------------------------------------------------------------
// received note on
// -----------------------------------------------------------------------------
void OnBleMidiNoteOn(byte channel, byte note, byte velocity) {
  N_DEBUG_PRINT(F("Incoming NoteOn  from channel:"));
  N_DEBUG_PRINT(channel);
  N_DEBUG_PRINT(F(" note:"));
  N_DEBUG_PRINT(note);
  N_DEBUG_PRINT(F(" velocity:"));
  N_DEBUG_PRINT(velocity);
  N_DEBUG_PRINTLN();
}


// -----------------------------------------------------------------------------
// received note off
// -----------------------------------------------------------------------------
void OnBleMidiNoteOff(byte channel, byte note, byte velocity) {
  N_DEBUG_PRINT(F("Incoming NoteOff from channel:"));
  N_DEBUG_PRINT(channel);
  N_DEBUG_PRINT(F(" note:"));
  N_DEBUG_PRINT(note);
  N_DEBUG_PRINT(F(" velocity:"));
  N_DEBUG_PRINT(velocity);
  N_DEBUG_PRINTLN();
}
