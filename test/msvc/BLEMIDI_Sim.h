#pragma once

BEGIN_BLEMIDI_NAMESPACE

template<typename T, int rawSize>
class Fifo {
public:
    const size_t size;				//speculative feature, in case it's needed

    Fifo() : size(rawSize)
    {
        flush();
    }

    T dequeue()
    {
        numberOfElements--;
        nextOut %= size;
        return raw[nextOut++];
    };

    bool enqueue(T element)
    {
        if (count() >= rawSize)
            return false;

        numberOfElements++;
        nextIn %= size;
        raw[nextIn] = element;
        nextIn++; //advance to next index

        return true;
    };

    T peek() const
    {
        return raw[nextOut % size];
    }

    void flush()
    {
        nextIn = nextOut = numberOfElements = 0;
    }

    // how many elements are currently in the FIFO?
    size_t count() { return numberOfElements; }

private:
    size_t numberOfElements;
    size_t nextIn;
    size_t nextOut;
    T raw[rawSize];
};

class BLEMIDI_Sim
{
private:
    static BLEMIDI_Transport<class BLEMIDI_Sim>* _bleMidiTransport;

    Fifo<byte, 64> mRxBuffer;

public:
    BLEMIDI_Sim()
    {
    }

    bool begin(const char*, BLEMIDI_Transport<class BLEMIDI_Sim>*);

    void end()
    {
    }

    void test()
    {

    }

    void write(uint8_t* buffer, size_t size)
    {
    }

    bool available(byte* pvBuffer)
    {
        if (mRxBuffer.count() > 0) {
            *pvBuffer = mRxBuffer.dequeue();

            return true;
        }

        return false;
    }

    void add(byte value)
    {
        mRxBuffer.enqueue(value);
    }

};

BLEMIDI_Transport<class BLEMIDI_Sim>* BLEMIDI_Sim::_bleMidiTransport = nullptr;

bool BLEMIDI_Sim::begin(const char* deviceName, BLEMIDI_Transport<class BLEMIDI_Sim>* bleMidiTransport)
{
    _bleMidiTransport = bleMidiTransport;

    byte sysExAndRealTime[] = { 0xB0, 0xF4,  // header + timestamp
                                0xF0,        // start SysEx
                                0x01, 0x02, 0x03, 0x04, // SysEx data
                                // RealTime message in the middle of a SysEx
                                0xF3, // timestampLow
                                0xFA, // Realtime msg: Start
                                // rest of sysex data
                                0x05, 0x06, 0x07, 0x08,
                                0xF4, // timestampLow
                                0xF7  // end of SysEx
                                };
    _bleMidiTransport->receive(sysExAndRealTime, sizeof(sysExAndRealTime));

    byte sysExPart[] = { 0xB0, 0xF4,  // header + timestamp
                      0xF0, // start SysEx
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // sysex data
                      0xF4, // timestampLow
                      0xF7 }; // end of SysEx

    _bleMidiTransport->receive(sysExPart, sizeof(sysExPart));

    byte sysExPart1[] = { 0xB0, 0xF4,  // header + timestamp
                          0xF0, // start SysEx
                            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };  // sysex data
    _bleMidiTransport->receive(sysExPart1, sizeof(sysExPart1));

    byte sysExPart2[] = { 0xB0,  // 1 byte header
                          0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, // sysex data (cont)
                          0xF4, // timestampLow
                          0xF7 }; // end of SysEx
    _bleMidiTransport->receive(sysExPart2, sizeof(sysExPart2));

    byte blePacketWithOneMIDIMessage[] = { 0xA8, 0xC0,
                                           0x90, 0x3E, 0x3E };
    _bleMidiTransport->receive(blePacketWithOneMIDIMessage, sizeof(blePacketWithOneMIDIMessage));

    byte blePacketWithTwoMIDIMessage[] = { 0xA8, 0xC0,
                                           0x90, 0x3E, 0x3E,
                                           0xC1,
                                           0x91, 0x3E, 0x3E };
    _bleMidiTransport->receive(blePacketWithTwoMIDIMessage, sizeof(blePacketWithTwoMIDIMessage));

    byte blePacketWithThreeMIDIMessage[] = { 0xA8, 0xC0,
                                           0x90, 0x3E, 0x3E,
                                           0xC1,
                                           0xF0,
                                            0x01, 0x02,
                                           0xC2,
                                           0xF7,
                                           0xC3,
                                           0x91, 0x3E, 0x3E };
    _bleMidiTransport->receive(blePacketWithThreeMIDIMessage, sizeof(blePacketWithThreeMIDIMessage));

    byte twoMIDIMessageWithRunningStatus[] = { 0xA9, 0xAD,
                                            0xD1, 0x74, //Full Midi 2 bytes(afterTouch)
                                            0x73, //running
                                            0xAE, //timeStamp
                                            0x72, //running after timeStamp
                                            0xAF, //timeStamp
                                            0x71, //running after timeStamp
                                            0x70,
                                            0x69,
                                            0x68,
                                            0xB2, //
                                            0x92, 0x36, 0x70, //Full Midi 3 bytes (noteOn)
                                            0xB3, //
                                            0x93, 0x37, 0x71,
                                            0x38, 0x72,
                                            0x39, 0x73,
                                            0xB4, //
                                            0x40, 0x74
    };
    _bleMidiTransport->receive(twoMIDIMessageWithRunningStatus, sizeof(twoMIDIMessageWithRunningStatus));

    byte twoMIDIMessageWithRunningStatusPlusSys[] = { 0xA9, 0xAD,
                                            0xD1, 0x74, //Full Midi 2 bytes(afterTouch)
                                            0x73, //running
                                            0xAE, //timeStamp
                                            0x72, //running after timeStamp
                                            0xAF, //timeStamp
                                            0x71, //running after timeStamp
                                            0x70,
                                            0x69,
                                            0x68,
                                            0xB2, //
                                            0xFA, // <- Sys START
                                            0xB2,
                                            0x92, 0x36, 0x70, //Full Midi 3 bytes (noteOn)
                                            0xB3, //
                                            0x93, 0x37, 0x71,
                                            0x38, 0x72,
                                            0xB3, //
                                            0xFC, // <- Sys STOP
                                            0xB3,
                                            0x39, 0x73,
                                            0xB4, //
                                            0x40, 0x74
    };
    _bleMidiTransport->receive(twoMIDIMessageWithRunningStatusPlusSys, sizeof(twoMIDIMessageWithRunningStatusPlusSys));

    return true;
}

/*! \brief Create an instance for nRF52 named <DeviceName>
*/
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Sim> BLE##Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Sim>, BLEMIDI_NAMESPACE::MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Sim> &)BLE##Name);

/*! \brief Create a default instance for nRF52 (Nano 33 BLE) named BLE-MIDI
*/
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
BLEMIDI_CREATE_INSTANCE("BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
