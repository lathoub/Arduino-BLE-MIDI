/*!
 *  @file		BLEMIDI.h
 */

#pragma once

#include <MIDI.h>
using namespace MIDI_NAMESPACE;

#include "BLE-MIDI_Settings.h"
#include "BLE-MIDI_Defs.h"
#include "BLE-MIDI_Namespace.h"

BEGIN_BLEMIDI_NAMESPACE

template<class T, class _Settings = DefaultSettings>
class BLEMIDITransport
{
    typedef _Settings Settings;

private:
    byte mRxBuffer[Settings::MaxBufferSize];
    unsigned mRxIndex = 0;

    byte mTxBuffer[Settings::MaxBufferSize];
    unsigned mTxIndex = 0;
    
    char mDeviceName[24];
    
private:
	T mBleClass;

public:    
    BLEMIDITransport(const char* deviceName)
	{
        strncpy(mDeviceName, deviceName, 24);
        
        mRxIndex = 0;
        mTxIndex = 0;
	}

public:	
    static const bool thruActivated = false;
	
    void begin()
    {
        mBleClass.begin(mDeviceName, this);

        // To communicate between the 2 cores.
        // Core_0 runs here, core_1 runs the BLE stack
        mRxQueue = xQueueCreate(Settings::MaxBufferSize, sizeof(uint8_t));
    }

    bool beginTransmission(MidiType)
    {
        getMidiTimestamp(&mTxBuffer[0], &mTxBuffer[1]);
        mTxIndex = 2;
        
        return true;
    }
    
    void write(byte inData)
    {
        // check for size! SysEx!!!
        if (false)
        {
            // should only happen from SysEx!
            // if we approach the end of the buffer, chop-up in segments until
            // we reach F7 (end of SysEx)
        }
        
        mTxBuffer[mTxIndex++] = inData;
    }

    void endTransmission()
    {
        mBleClass.write(mTxBuffer, mTxIndex);
        mTxIndex = 0;
    }
 
    byte read()
    {
        return mRxBuffer[--mRxIndex];
    }

    unsigned available()
    {
        uint8_t byte;
        auto succes = xQueueReceive(mRxQueue, &byte, 0); // return immediately when the queue is empty
        if (!succes) return mRxIndex;

        mRxBuffer[mRxIndex++] = byte;

        return mRxIndex;
    }
    
public:
    QueueHandle_t mRxQueue;
    
protected:
    /*
     The first byte of all BLE packets must be a header byte. This is followed by timestamp bytes and MIDI messages.
     
     Header Byte
     bit 7     Set to 1.
     bit 6     Set to 0. (Reserved for future use)
     bits 5-0  timestampHigh:Most significant 6 bits of timestamp information.
     The header byte contains the topmost 6 bits of timing information for MIDI events in the BLE
     packet. The remaining 7 bits of timing information for individual MIDI messages encoded in a
     packet is expressed by timestamp bytes.

     Timestamp Byte
     bit 7       Set to 1.
     bits 6-0    timestampLow: Least Significant 7 bits of timestamp information.
     
     The 13-bit timestamp for the first MIDI message in a packet is calculated using 6 bits from the
     header byte and 7 bits from the timestamp byte.
     
     Timestamps are 13-bit values in milliseconds, and therefore the maximum value is 8,191 ms.
     Timestamps must be issued by the sender in a monotonically increasing fashion.
     timestampHigh is initially set using the lower 6 bits from the header byte while the timestampLow is
     formed of the lower 7 bits from the timestamp byte. Should the timestamp value of a subsequent
     MIDI message in the same packet overflow/wrap (i.e., the timestampLow is smaller than a
     preceding timestampLow), the receiver is responsible for tracking this by incrementing the
     timestampHigh by one (the incremented value is not transmitted, only understood as a result of the
     overflow condition).
     
     In practice, the time difference between MIDI messages in the same BLE packet should not span
     more than twice the connection interval. As a result, a maximum of one overflow/wrap may occur
     per BLE packet.
     
     Timestamps are in the sender’s clock domain and are not allowed to be scheduled in the future.
     Correlation between the receiver’s clock and the received timestamps must be performed to
     ensure accurate rendering of MIDI messages, and is not addressed in this document.
     */

    /*
     Calculating a Timestamp
     To calculate the timestamp, the built-in millis() is used.
     The BLE standard only specifies 13 bits worth of millisecond data though,
     so it’s bitwise anded with 0x1FFF for an ever repeating cycle of 13 bits.
     This is done right after a MIDI message is detected. It’s split into a 6 upper bits, 7 lower bits,
     and the MSB of both bytes are set to indicate that this is a header byte.
     Both bytes are placed into the first two position of an array in preparation for a MIDI message.
     */
    static void getMidiTimestamp (uint8_t *header, uint8_t *timestamp)
    {
        auto currentTimeStamp = millis() & 0x01FFF;
        
        *header = ((currentTimeStamp >> 7) & 0x3F) | 0x80;        // 6 bits plus MSB
        *timestamp = (currentTimeStamp & 0x7F) | 0x80;            // 7 bits plus MSB
    }
    
    static void setMidiTimestamp (uint8_t header, uint8_t *timestamp)
    {
    }
    
public:
	// callbacks
	void(*_connectedCallback)() = nullptr;
	void(*_disconnectedCallback)() = nullptr;

public:
	void onConnected(void(*fptr)()) {
		_connectedCallback = fptr;
	}

	void onDisconnected(void(*fptr)()) {
		_disconnectedCallback = fptr;
	}

};

END_BLEMIDI_NAMESPACE

 struct MySettings : public MIDI_NAMESPACE::DefaultSettings
 {
    static const bool Use1ByteParsing = false;
 };

/*! \brief Create an instance of the library
 */
#define BLEMIDI_CREATE_INSTANCE(Type, DeviceName, Name)     \
BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32> BLE##Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32>, MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32> &)BLE##Name);

 /*! \brief Create an instance for ESP32 named <DeviceName>
 */
#define BLEMIDI_CREATE_ESP32_INSTANCE(DeviceName) \
BLEMIDI_CREATE_INSTANCE(BLEMIDI_NAMESPACE::BLEMIDI_ESP32, DeviceName, MIDI);

 /*! \brief Create a default instance for ESP32 named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_ESP32_INSTANCE() \
BLEMIDI_CREATE_ESP32_INSTANCE("BLE-MIDI")
