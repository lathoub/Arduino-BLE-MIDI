/*!
 *  @file		BLEMIDI_Transport.h
 */

#pragma once

#include <MIDI.h>

#include "BLEMIDI_Settings.h"
#include "BLEMIDI_Defs.h"
#include "BLEMIDI_Namespace.h"

BEGIN_BLEMIDI_NAMESPACE

using namespace MIDI_NAMESPACE;

// As specified in
// Specification for MIDI over Bluetooth Low Energy (BLE-MIDI)
// Version 1.0a, November 1, 2015
// 3. BLE Service and Characteristics Definitions
static const char *const SERVICE_UUID        = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
static const char *const CHARACTERISTIC_UUID = "7772e5db-3868-4112-a1a9-f2669d106bf3";

#define MIDI_TYPE 0x80

template <class T, class _Settings /*= CommonBLEDefaultSettings*/>
class BLEMIDI_Transport
{
private:
    byte mRxBuffer[_Settings::MaxBufferSize];
    unsigned mRxIndex = 0;

    byte mTxBuffer[_Settings::MaxBufferSize]; // minimum 5 bytes
    unsigned mTxIndex = 0;

    char mDeviceName[24];

    uint8_t mTimestampLow;

private:
    T mBleClass;

public:
    BLEMIDI_Transport(const char *deviceName)
    {
        strncpy(mDeviceName, deviceName, sizeof(mDeviceName));

        mRxIndex = 0;
        mTxIndex = 0;
    }

public:
    static const bool thruActivated = false;

    void begin()
    {
        mBleClass.begin(mDeviceName, this);
    }

    void end()
    {
        mBleClass.end();
    }

    bool beginTransmission(MIDI_NAMESPACE::MidiType type)
    {
        getMidiTimestamp(&mTxBuffer[0], &mTxBuffer[1]);
        mTxIndex = 2;
        mTimestampLow = mTxBuffer[1]; // or generate new ?

        return true;
    }

    void write(byte inData)
    {
        if (mTxIndex >= sizeof(mTxBuffer))
        {
            mBleClass.write(mTxBuffer, sizeof(mTxBuffer));
            mTxIndex = 1; // keep header
        }

        mTxBuffer[mTxIndex++] = inData;
    }

    void endTransmission()
    {
        if (mTxBuffer[mTxIndex - 1] == SystemExclusiveEnd)
        {
            if (mTxIndex >= sizeof(mTxBuffer))
            {
                mBleClass.write(mTxBuffer, mTxIndex - 1);

                mTxIndex = 1;                          // keep header
                mTxBuffer[mTxIndex++] = mTimestampLow; // or generate new ?
            }
            else
            {
                mTxBuffer[mTxIndex - 1] = mTimestampLow; // or generate new ?
            }
            mTxBuffer[mTxIndex++] = SystemExclusiveEnd;
        }

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
        auto success = mBleClass.available(&byte);
        if (!success)
            return mRxIndex;

        mRxBuffer[mRxIndex++] = byte;
        return mRxIndex;
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
    static void getMidiTimestamp(uint8_t *header, uint8_t *timestamp)
    {
        auto currentTimeStamp = millis() & 0x01FFF;

        *header = ((currentTimeStamp >> 7) & 0x3F) | 0x80; // 6 bits plus MSB
        *timestamp = (currentTimeStamp & 0x7F) | 0x80;     // 7 bits plus MSB
    }

    static uint16_t setMidiTimestamp(uint8_t header, uint8_t timestamp)
    {
        auto timestampHigh = 0x3f & header;
        auto timestampLow = 0x7f & timestamp;
        return (timestampLow + (timestampHigh << 7));
    }

public:
    // callbacks
    void (*_connectedCallback)() = nullptr;
    void (*_disconnectedCallback)() = nullptr;

    BLEMIDI_Transport &setName(const char *deviceName)
    {
        strncpy(mDeviceName, deviceName, sizeof(mDeviceName));
        return *this;
    };

public:
    BLEMIDI_Transport &setHandleConnected(void (*fptr)())
    {
        _connectedCallback = fptr;
        return *this;
    }

    BLEMIDI_Transport &setHandleDisconnected(void (*fptr)())
    {
        _disconnectedCallback = fptr;
        return *this;
    }

/*
    The general form of a MIDI message follows:
    n-byte MIDI Message
    Byte 0            MIDI message Status byte, Bit 7 is Set to 1.
    Bytes 1 to n-1    MIDI message Data bytes, if n > 1. Bit 7 is Set to 0
    There are two types of MIDI messages that can appear in a single packet: full MIDI messages and
    Running Status MIDI messages. Each is encoded differently.
    A full MIDI message is simply the MIDI message with the Status byte included.
    A Running Status MIDI message is a MIDI message with the Status byte omitted. Running Status
    MIDI messages may only be placed in the data stream if the following criteria are met:
    1.  The original MIDI message is 2 bytes or greater and is not a System Common or System
    Real-Time message.
    2.  The omitted Status byte matches the most recently preceding full MIDI message’s Status
    byte within the same BLE packet.
    In addition, the following rules apply with respect to Running Status:
    1.  A Running Status MIDI message is allowed within the packet after at least one full MIDI
    message.
    2.  Every MIDI Status byte must be preceded by a timestamp byte. Running Status MIDI
    messages may be preceded by a timestamp byte. If a Running Status MIDI message is not
    preceded by a timestamp byte, the timestamp byte of the most recently preceding message
    in the same packet is used.
    3.  System Common and System Real-Time messages do not cancel Running Status if
    interspersed between Running Status MIDI messages. However, a timestamp byte must
    precede the Running Status MIDI message that follows.
    4.  The end of a BLE packet does cancel Running Status.
    In the MIDI 1.0 protocol, System Real-Time messages can be sent at any time and may be
    inserted anywhere in a MIDI data stream, including between Status and Data bytes of any other
    MIDI messages. In the MIDI BLE protocol, the System Real-Time messages must be deinterleaved
    from other messages – except for System Exclusive messages.
    */

/**
   * If #define RUNNING_ENABLE is commented/disabled, it will transform all incoming runningStatus messages in full midi messages.
   * Else, it will put in the buffer the same info that it had received (runningStatus will be not transformated).
   * It recommend not use runningStatus by default. Only use if parser accepts runningStatus and your application has a so high transmission rate.
   */
#define RUNNING_ENABLE

    void receive(byte *buffer, size_t length)
    {
        // Pointers used to search through payload.
        int lPtr = 0;
        int rPtr = 0;

        // lastStatus used to capture runningStatus
        byte lastStatus;
        // previousStatus used to continue a runningStatus interrupted by a timeStamp or a System Message.
        byte previousStatus = InvalidType;

        byte headerByte = buffer[lPtr++];

        auto timestampHigh = 0x3f & headerByte;
        timestampHigh = timestampHigh; // <-- This line is for avoid Warning message due it is unused
        byte timestampByte = buffer[lPtr++];
        uint16_t timestamp = 0;
        timestamp = timestamp; // <-- This line is for avoid Warning message due it is unused
        bool sysExContinuation = false;
        bool runningStatusContinuation = false;

        if (timestampByte >= MIDI_TYPE) // if bit 7 is 1, it's a timestampByte
        {
            timestamp = setMidiTimestamp(headerByte, timestampByte);
            // what do to with the timestamp?
        }
        else // if bit 7 is 0, it's the Continuation of a previous SysEx
        {
            sysExContinuation = true;
            lPtr--; // the second byte is part of the SysEx
        }

        //While statement contains incrementing pointers and breaks when buffer size exceeded.
        while (true)
        {
            lastStatus = buffer[lPtr];

            if (previousStatus == InvalidType)
            {
                if ((lastStatus < MIDI_TYPE) && !sysExContinuation)
                    return; // Status message not present and it is not a runningStatus continuation, bail
            }
            else if (lastStatus < MIDI_TYPE)
            {
                lastStatus = previousStatus;
                runningStatusContinuation = true;
            }

            // Point to next non-data byte
            rPtr = lPtr;
            while ((buffer[rPtr + 1] < MIDI_TYPE) && (rPtr < (length - 1)))
                rPtr++;

            if (!runningStatusContinuation)
            {
                // If not System Common or System Real-Time, send it as running status

                auto midiType = lastStatus & 0xF0;
                if (sysExContinuation)
                    midiType = SystemExclusive;

                switch (midiType)
                {
                case NoteOff:
                case NoteOn:
                case AfterTouchPoly:
                case ControlChange:
                case PitchBend:
#ifdef RUNNING_ENABLE
                    mBleClass.add(lastStatus);
#endif
                    for (auto i = lPtr; i < rPtr; i = i + 2)
                    {
#ifndef RUNNING_ENABLE
                        mBleClass.add(lastStatus);
#endif
                        mBleClass.add(buffer[i + 1]);
                        mBleClass.add(buffer[i + 2]);
                    }
                    break;
                case ProgramChange:
                case AfterTouchChannel:
#ifdef RUNNING_ENABLE
                    mBleClass.add(lastStatus);
#endif
                    for (auto i = lPtr; i < rPtr; i = i + 1)
                    {
#ifndef RUNNING_ENABLE
                        mBleClass.add(lastStatus);
#endif
                        mBleClass.add(buffer[i + 1]);
                    }
                    break;
                case SystemExclusive:
                    mBleClass.add(lastStatus);
                    for (auto i = lPtr; i < rPtr; i++)
                        mBleClass.add(buffer[i + 1]);

                    break;

                default:
                    break;
                }
            }
            else
            {
#ifndef RUNNING_ENABLE
                auto midiType = lastStatus & 0xF0;
                switch (midiType)
                {
                case NoteOff:
                case NoteOn:
                case AfterTouchPoly:
                case ControlChange:
                case PitchBend:
                    //3 bytes full Midi -> 2 bytes runningStatus
                    for (auto i = lPtr; i <= rPtr; i = i + 2)
                    {
                        mBleClass.add(lastStatus);
                        mBleClass.add(buffer[i]);
                        mBleClass.add(buffer[i + 1]);
                    }
                    break;
                case ProgramChange:
                case AfterTouchChannel:
                    //2 bytes full Midi -> 1 byte runningStatus
                    for (auto i = lPtr; i <= rPtr; i = i + 1)
                    {
                        mBleClass.add(lastStatus);
                        mBleClass.add(buffer[i]);
                    }
                    break;

                default:
                    break;
                }
#else
                mBleClass.add(lastStatus);
                for (auto i = lPtr; i <= rPtr; i++)
                    mBleClass.add(buffer[i]);
#endif
                runningStatusContinuation = false;
            }

            if (++rPtr >= length)
                return; // end of packet

            if (lastStatus < SystemExclusive) //exclude System Message. They must not be RunningStatus
            {
                previousStatus = lastStatus;
            }

            timestampByte = buffer[rPtr++];
            if (timestampByte >= MIDI_TYPE) // is bit 7 set?
            {
                timestamp = setMidiTimestamp(headerByte, timestampByte);
                // what do to with the timestamp?
            }

            // Point to next status
            lPtr = rPtr;
            if (lPtr >= length)
                return; //end of packet
        }
    }
};

END_BLEMIDI_NAMESPACE
