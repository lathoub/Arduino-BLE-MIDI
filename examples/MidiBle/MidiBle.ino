#include <MIDI.h>

#include <midi_bleTransport.h>
#include <Ble_esp32.h>

bleMidi::BluetoothEsp32 sBluetoothEsp32;
bleMidi::BleMidiTransport<bleMidi::BluetoothEsp32> bm((bleMidi::BluetoothEsp32&) sBluetoothEsp32);
midi::MidiInterface<bleMidi::BleMidiTransport<bleMidi::BluetoothEsp32>> MIDI((bleMidi::BleMidiTransport<bleMidi::BluetoothEsp32>&)bm);

unsigned long t0 = millis();
bool isConnected = false;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // Serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("booting"));

  MIDI.begin("Huzzah BLE MIDI", 1);

  bm.onConnected(OnBleMidiConnected);
  bm.onDisconnected(OnBleMidiDisconnected);

  MIDI.setHandleNoteOn(OnBleMidiNoteOn);
  MIDI.setHandleNoteOff(OnBleMidiNoteOff);

  Serial.println(F("ready"));
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
    MIDI.sendNoteOff(60, 127, 1);
  }

}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnBleMidiConnected() {
  Serial.println(F("Connected"));
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