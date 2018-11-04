#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "AbstractMidiInterface.h"
using namespace Midi;

BEGIN_BLEMIDI_NAMESPACE

class BleMidiInterface : public AbstractMidiInterface
{
protected:
    // ESP32
    BLEServer  *pServer;
    BLEAdvertising *pAdvertising;
    BLECharacteristic *pCharacteristic;
    
    bool _connected;
    
public:
    // callbacks
    void(*_connectedCallback)() = NULL;
    void(*_disconnectedCallback)() = NULL;
    
protected:
    void getMidiTimestamp (uint8_t *header, uint8_t *timestamp)
    {
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
        auto currentTimeStamp = millis() & 0x01FFF;
        
        *header = ((currentTimeStamp >> 7) & 0x3F) | 0x80;        // 6 bits plus MSB
        *timestamp = (currentTimeStamp & 0x7F) | 0x80;            // 7 bits plus MSB
    }
    
    void sendChannelMessage1(byte type, byte channel, byte data1)
    {
        uint8_t midiPacket[4];
        
        getMidiTimestamp(&midiPacket[0], &midiPacket[1]);
        midiPacket[2] = (type & 0xf0) | ((channel - 1) & 0x0f);
        midiPacket[3] = data1;
        pCharacteristic->setValue(midiPacket, 4);
        pCharacteristic->notify();
    }
    
    void sendChannelMessage2(byte type, byte channel, byte data1, byte data2)
    {
        uint8_t midiPacket[5];
        
        getMidiTimestamp(&midiPacket[0], &midiPacket[1]);
        midiPacket[2] = (type & 0xf0) | ((channel - 1) & 0x0f);
        midiPacket[3] = data1;
        midiPacket[4] = data2;
        pCharacteristic->setValue(midiPacket, 5);
        pCharacteristic->notify();
    }
    
    void sendSystemCommonMessage1(byte type, byte data1)
    {
        uint8_t midiPacket[4];
        
        getMidiTimestamp(&midiPacket[0], &midiPacket[1]);
        midiPacket[2] = type;
        midiPacket[3] = data1;
        pCharacteristic->setValue(midiPacket, 4);
        pCharacteristic->notify();
    }
    
    void sendSystemCommonMessage2(byte type, byte data1, byte data2)
    {
        uint8_t midiPacket[5];
        
        getMidiTimestamp(&midiPacket[0], &midiPacket[1]);
        midiPacket[2] = type;
        midiPacket[3] = data1;
        midiPacket[4] = data2;
        pCharacteristic->setValue(midiPacket, 5);
        pCharacteristic->notify();
    }
    
    void sendRealTimeMessage(byte type)
    {
        uint8_t midiPacket[3];
        
        getMidiTimestamp(&midiPacket[0], &midiPacket[1]);
        midiPacket[2] = type;
        pCharacteristic->setValue(midiPacket, 3);
        pCharacteristic->notify();
    }

public:
    BleMidiInterface()
    {
    }
    
     ~BleMidiInterface()
    {
    }
    
    inline bool begin(const char* deviceName);
    
    inline void receive(uint8_t *buffer, uint8_t bufferSize);
    
    void onConnected(void(*fptr)()) {
        _connected = true;
        _connectedCallback = fptr;
    }
    void onDisconnected(void(*fptr)()) {
        _connected = false;
        _disconnectedCallback = fptr;
    }

    //
    void send(MidiType type, DataByte data1, DataByte data2, Channel channel) {
        
    }

};

class MyServerCallbacks: public BLEServerCallbacks {
public:
    MyServerCallbacks(BleMidiInterface* bleMidiInterface) {
        _bleMidiInterface = bleMidiInterface;
    }
protected:
    BleMidiInterface* _bleMidiInterface;

    void onConnect(BLEServer* pServer) {
        if (_bleMidiInterface->_connectedCallback)
            _bleMidiInterface->_connectedCallback();
    };
    
    void onDisconnect(BLEServer* pServer) {
        if (_bleMidiInterface->_disconnectedCallback)
            _bleMidiInterface->_disconnectedCallback();
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
public:
    MyCharacteristicCallbacks(BleMidiInterface* bleMidiInterface) {
        _bleMidiInterface = bleMidiInterface;
    }
protected:
    BleMidiInterface* _bleMidiInterface;

    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            _bleMidiInterface->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
        }
    }
};

bool BleMidiInterface::begin(const char* deviceName)
{
    BLEDevice::init(deviceName);
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(this));
    
    // Create the BLE Service
    auto pService = pServer->createService(BLEUUID(SERVICE_UUID));
    
    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                                                     BLEUUID(CHARACTERISTIC_UUID),
                                                     BLECharacteristic::PROPERTY_READ   |
                                                     BLECharacteristic::PROPERTY_WRITE  |
                                                     BLECharacteristic::PROPERTY_NOTIFY |
                                                     BLECharacteristic::PROPERTY_WRITE_NR
                                                     );
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(this));
    // Start the service
    pService->start();
    
    auto advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x04);
    advertisementData.setCompleteServices(BLEUUID(SERVICE_UUID));
    advertisementData.setName(deviceName);
    
    // Start advertising
    pAdvertising = pServer->getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    
    return true;
}

void BleMidiInterface::receive(uint8_t *buffer, uint8_t bufferSize)
{
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
    Channel channel;
    MidiType command;
    
    //Pointers used to search through payload.
    uint8_t lPtr = 0;
    uint8_t rPtr = 0;
    //Decode first packet -- SHALL be "Full MIDI message"
    lPtr = 2; //Start at first MIDI status -- SHALL be "MIDI status"
    //While statement contains incrementing pointers and breaks when buffer size exceeded.
    while (1) {
        //lastStatus used to capture runningStatus
        auto lastStatus = buffer[lPtr];
        if ( (buffer[lPtr] < 0x80) ) {
            //Status message not present, bail
            return;
        }
        
        command = getTypeFromStatusByte(lastStatus);
        channel = getChannelFromStatusByte(lastStatus);
        
        //Point to next non-data byte
        rPtr = lPtr;
        while ( (buffer[rPtr + 1] < 0x80) && (rPtr < (bufferSize - 1)) ) {
            rPtr++;
        }
        //look at l and r pointers and decode by size.
        if ( rPtr - lPtr < 1 ) {
            //Time code or system
            //        MIDI.send(command, 0, 0, channel);
        } else if ( rPtr - lPtr < 2 ) {
            //        MIDI.send(command, buffer[lPtr + 1], 0, channel);
        } else if ( rPtr - lPtr < 3 ) {

            // TODO: switch for type
            
            if (_noteOnCallback)
                _noteOnCallback(0, 1, 2);

            //        MIDI.send(command, buffer[lPtr + 1], buffer[lPtr + 2], channel);
        } else {
            //Too much data
            //If not System Common or System Real-Time, send it as running status
            switch ( buffer[lPtr] & 0xF0 )
            {
                case 0x80:
                case 0x90:
                case 0xA0:
                case 0xB0:
                case 0xE0:
                    for (int i = lPtr; i < rPtr; i = i + 2) {
                        //                   MIDI.send(command, buffer[i + 1], buffer[i + 2], channel);
                    }
                    break;
                case 0xC0:
                case 0xD0:
                    for (int i = lPtr; i < rPtr; i = i + 1) {
                        //                   MIDI.send(command, buffer[i + 1], 0, channel);
                    }
                    break;
                default:
                    break;
            }
        }
        //Point to next status
        lPtr = rPtr + 2;
        if (lPtr >= bufferSize) {
            //end of packet
            return;
        }
    }
}

END_BLEMIDI_NAMESPACE
