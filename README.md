# Arduino BLE-MIDI Transport 
This library implements the BLE-MIDI transport layer for the [FortySevenEffects Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) 

## Installation
This library depends on the [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library).

When installing this library from the Arduino IDE, the dependency be downloaded and installed in the same directory as this library. (Thanks to the `depends` clause in `library.properties`)

When manually installing this library, you have to manually download [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) from github and install it in the same directory as this library - without this additional install, this library will not be able to compile. 

## Usage
### Basic / Default
```cpp
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
...
BLEMIDI_CREATE_DEFAULT_INSTANCE()
...
void setup()
{
   MIDI.begin();
...
void loop()
{
   MIDI.read();
```
will create a instance named `BLEMIDI` and listens to incoming MIDI.

### using NimBLE for ESP32 with a custom name and turns LED on when its connected

```cpp
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32_NimBLE.h>
...
BLEMIDI_CREATE_INSTANCE("CustomName", MIDI)
...
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  BLEMIDI.setHandleConnected(OnConnected);
  BLEMIDI.setHandleDisconnected(OnDisconnected);
  
  MIDI.begin();
...
void loop()
{
  MIDI.read();
...
void OnConnected() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void OnDisconnected() {
  digitalWrite(LED_BUILTIN, LOW);
}
   
```
will create a instance named `BLEMIDI` and listens to incoming MIDI.

## Tested boards/modules
- ESP32
- ESP32 NimBLE
- Arduino NANO 33 BLE

## Other Transport protocols:
The libraries below  the same calling mechanism (API), making it easy to interchange the transport layer.
- [Arduino AppleMIDI Transport](https://github.com/lathoub/Arduino-AppleMIDI-Library)
- [Arduino USB-MIDI  Transport](https://github.com/lathoub/USB-MIDI)
- [Arduino ipMIDI  Transport](https://github.com/lathoub/Arduino-ipMIDI)
