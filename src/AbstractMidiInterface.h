/*!
 *  @file		BleMidi.h
 */

#pragma once

#include "utility/MIDI_Defs.h"

BEGIN_BLEMIDI_NAMESPACE

class AbstractMidiInterface
{
protected:
    int _runningStatus;
    bool _thruActivated;
    
public:
    AbstractMidiInterface()
    {
    }
    
protected:
    void (*_noteOnCallback)(byte channel, byte note, byte velocity) = NULL;
    void (*_noteOffCallback)(byte channel, byte note, byte velocity) = NULL;
    void (*_afterTouchPolyCallback)(byte channel, byte note, byte velocity) = NULL;
    void (*_controlChangeCallback)(byte channel, byte, byte) = NULL;
    void (*_programChangeCallback)(byte channel, byte) = NULL;
    void (*_afterTouchChannelCallback)(byte channel, byte) = NULL;
    void (*_pitchBendCallback)(byte channel, int) = NULL;
    void (*_songPositionCallback)(unsigned short beats) = NULL;
    void (*_songSelectCallback)(byte songnumber) = NULL;
    void (*_tuneRequestCallback)(void) = NULL;
    void (*_timeCodeQuarterFrameCallback)(byte data) = NULL;
    void (*_sysExCallback)(const byte* array, uint16_t size) = NULL;
    void (*_clockCallback)(void) = NULL;
    void (*_startCallback)(void) = NULL;
    void (*_continueCallback)(void) = NULL;
    void (*_stopCallback)(void) = NULL;
    void (*_activeSensingCallback)(void) = NULL;
    void (*_resetCallback)(void) = NULL;

public:
    // sending
    void sendNoteOn(DataByte note, DataByte velocity, Channel channel) {
        send(Type::NoteOn, channel, note, velocity);
    }
    
    void sendNoteOff(DataByte note, DataByte velocity, Channel channel) {
        send(Type::NoteOff, channel, note, velocity);
    }
    
    void sendProgramChange(DataByte number, Channel channel) {
        send(Type::ProgramChange, number, 0, channel);
   }
    
    void sendControlChange(DataByte number, DataByte value, Channel channel) {
        send(Type::ControlChange, number, value, channel);
   }
    
    void sendPitchBend(int value, Channel channel) {
        const unsigned bend = unsigned(value - int(MIDI_PITCHBEND_MIN));
        send(Type::PitchBend, (bend & 0x7f), (bend >> 7) & 0x7f, channel);
    }
    
    void sendPitchBend(double pitchValue, Channel channel) {
        const int scale = pitchValue > 0.0 ? MIDI_PITCHBEND_MAX : MIDI_PITCHBEND_MIN;
        const int value = int(pitchValue * double(scale));
        sendPitchBend(value, channel);
    }
    
    void sendPolyPressure(DataByte note, DataByte pressure, Channel channel) {
        send(Type::AfterTouchPoly, note, pressure, channel);
    }
    
    void sendAfterTouch(DataByte pressure, Channel channel) {
        send(Type::AfterTouchChannel, pressure, 0, channel);
    }
    
    void sendAfterTouch(DataByte note, DataByte pressure, Channel channel) {
        send(Type::AfterTouchChannel, note, pressure, channel);
    }
    
    void sendSysEx(const byte*, uint16_t inLength) {
    }
    
    void sendTimeCodeQuarterFrame(DataByte typeNibble, DataByte valuesNibble) {
    }
    
    void sendTimeCodeQuarterFrame(DataByte data) {
    }
    
    void sendSongPosition(unsigned short beats) {
    }
    
    void sendSongSelect(DataByte number) {
    }
    
    void sendTuneRequest() {
    }
    
    void sendActiveSensing() {
    }
    
    void sendStart() {
    }
    
    void sendContinue() {
    }
    
    void sendStop() {
    }
    
    void sendReset() {
    }
    
    void sendClock() {
    }
    
    void sendTick() {
    }

    //receiving
    void OnReceiveNoteOn(void (*fptr)(byte channel, byte note, byte velocity)) {
        _noteOnCallback = fptr;
    }
    void OnReceiveNoteOff(void (*fptr)(byte channel, byte note, byte velocity)) {
        _noteOffCallback = fptr;
    }
    void OnReceiveAfterTouchPoly(void (*fptr)(byte channel, byte note, byte pressure)) {
        _afterTouchPolyCallback = fptr;
    }
    void OnReceiveControlChange(void (*fptr)(byte channel, byte number, byte value)) {
        _controlChangeCallback = fptr;
    }
    void OnReceiveProgramChange(void (*fptr)(byte channel, byte number)) {
        _programChangeCallback = fptr;
    }
    void OnReceiveAfterTouchChannel(void (*fptr)(byte channel, byte pressure)) {
        _afterTouchChannelCallback = fptr;
    }
    void OnReceivePitchBend(void (*fptr)(byte channel, int bend)) {
        _pitchBendCallback = fptr;
    }
    void OnReceiveSysEx(void (*fptr)(const byte * data, uint16_t size)) {
        _sysExCallback = fptr;
    }
    void OnReceiveTimeCodeQuarterFrame(void (*fptr)(byte data)) {
        _timeCodeQuarterFrameCallback = fptr;
    }
    void OnReceiveSongPosition(void (*fptr)(unsigned short beats)) {
        _songPositionCallback = fptr;
    }
    void OnReceiveSongSelect(void (*fptr)(byte songnumber)) {
        _songSelectCallback = fptr;
    }
    void OnReceiveTuneRequest(void (*fptr)(void)) {
        _tuneRequestCallback = fptr;
    }
    void OnReceiveClock(void (*fptr)(void)) {
        _clockCallback = fptr;
    }
    void OnReceiveStart(void (*fptr)(void)) {
        _startCallback = fptr;
    }
    void OnReceiveContinue(void (*fptr)(void)) {
        _continueCallback = fptr;
    }
    void OnReceiveStop(void (*fptr)(void)) {
        _stopCallback = fptr;
    }
    void OnReceiveActiveSensing(void (*fptr)(void)) {
        _activeSensingCallback = fptr;
    }
    void OnReceiveReset(void (*fptr)(void)) {
        _resetCallback = fptr;
    }
    
protected:
    //
    virtual void send(Type type, DataByte data1, DataByte data2, Channel channel) = 0;
};

END_BLEMIDI_NAMESPACE
