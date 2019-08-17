/*!
 *  @file		midi_bleTransport.h
 */

#pragma once

#include "utility/midi_bleSettings.h"
#include "utility/midi_bleDefs.h"

#include <midi_RingBuffer.h>

BEGIN_BLEMIDI_NAMESPACE

template<class BleClass>
class BleMidiTransport
{
private:
    midi::RingBuffer<byte, 44> mRxBuffer;
    
    byte mTxBuffer[44];
    unsigned mTxIndex = 0;
    
private:
	BleClass& mBleClass;

public:
	inline BleMidiTransport(BleClass& inBleClass)
		: mBleClass(inBleClass)
	{
	}

	inline ~BleMidiTransport() {}

    inline bool begin(int baudrate) {} // n/a
    inline bool begin(const char* deviceName) { return mBleClass.begin(deviceName, this); }

    inline unsigned available() { return mRxBuffer.getLength();  }
    inline byte read() { return mRxBuffer.read(); }
    
    inline void beginWrite()
    {
        getMidiTimestamp(&mTxBuffer[0], &mTxBuffer[1]);
        mTxIndex = 2;
    }
    inline void write(byte inData)
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
    inline void endWrite()
    {
        mBleClass.write(mTxBuffer, mTxIndex);
    }
 
public:
    void receive(uint8_t* buffer, uint8_t length)
    {
        // TODO: check for size!! (SysEx!!)
        mRxBuffer.read(buffer, length);
    }

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
    inline static void getMidiTimestamp (uint8_t *header, uint8_t *timestamp)
    {
        auto currentTimeStamp = millis() & 0x01FFF;
        
        *header = ((currentTimeStamp >> 7) & 0x3F) | 0x80;        // 6 bits plus MSB
        *timestamp = (currentTimeStamp & 0x7F) | 0x80;            // 7 bits plus MSB
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
