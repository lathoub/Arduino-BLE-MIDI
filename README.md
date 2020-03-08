# Experimental
   
# Arduino BLE-MIDI Transport 
This library implements the BLE-MIDI transport layer for the [FortySevenEffects Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) 

## Installation
This library depends on the [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library).

When installing this library from the Arduino IDE, the dependency be downloaded and installed in the same directory as this library. (Thanks to the `depends` clause in `library.properties`)

When manually installing this library, you have to manually download [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) from github and install it in the same directory as this library - without this additional install, this library will not be able to compile. 

## Usage
### Basic / Default
```cpp
#include <BLE-MIDI.h>
#include <hardware/BLE-MIDI_ESP32.h>
...
BLEMIDI_CREATE_DEFAULT_ESP32_INSTANCE();
...
void setup()
{
   MIDI.begin(1);
...
void loop()
{
   MIDI.read();
```
will create a instance named `bleMIDI` and listens to incoming MIDI on channel 1.

## Tested boards/modules
- Adafruit Huzzah32 feather

## Other Transport protocols:
The libraries below  the same calling mechanism (API), making it easy to interchange the transport layer.
- [Arduino AppleMIDI Transport](https://github.com/lathoub/Arduino-AppleMIDI-Library)
- [Arduino USB-MIDI  Transport](https://github.com/lathoub/USB-MIDI)
- [Arduino ipMIDI  Transport](https://github.com/lathoub/Arduino-ipMIDI)
