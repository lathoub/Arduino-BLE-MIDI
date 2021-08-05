#include "Arduino.h"
#include <BLEMIDI_Transport.h>
#include "BLEMIDI_Sim.h"

BLEMIDI_CREATE_DEFAULT_INSTANCE()

void begin()
{
	MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
		std::cout << std::hex << "NoteOn from Channel:" << (int)channel << " Note:" << (int)note << " Velocity:" << (int)velocity << std::endl;
	});
	MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
		std::cout << std::hex << "NoteOff from Channel:" << (int)channel << " Note:" << (int)note << " Velocity:" << (int)velocity << std::endl;
	});
	MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
		std::cout << std::hex << "NoteOff from Channel:" << (int)channel << " Note:" << (int)note << " Velocity:" << (int)velocity << std::endl;
	});
	MIDI.setHandleControlChange([](byte channel, byte v1, byte v2) {
		std::cout << std::hex << "ControlChange from Channel:" << (int)channel << " v1:" << (int)v1 << " v2:" << (int)v2 << std::endl;
	});
	MIDI.setHandleProgramChange([](byte channel, byte v1) {
		std::cout << std::hex << "ProgramChange from Channel:" << (int)channel << " v1:" << (int)v1 << std::endl;
	});
	MIDI.setHandlePitchBend([](byte channel, int v1) {
		std::cout << std::hex << "PitchBend from Channel:" << (int)channel << " v1:" << (int)v1 << std::endl;
	});
	MIDI.setHandleSystemExclusive([](byte* data, unsigned length) {
		std::cout << std::hex << "SysEx:";
		for (uint16_t i = 0; i < length; i++)
			std::cout << std::hex << "0x" << (int)data[i] << " ";
		std::cout << std::endl;
	});

	MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
	MIDI.read();
}