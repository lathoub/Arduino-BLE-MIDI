// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

typedef unsigned int word;
typedef uint8_t boolean;
typedef unsigned char byte;

#define CHECK_BIT(var,pos) (!!((var) & (1<<(pos))))

void transmitMIDIonDIN(byte status, byte data1, byte data2)
{
    std::cout << "0x" << std::hex << (int)status;

    if (data1 > 0)
        std::cout << " 0x" << std::hex << (int)data1;
    if (data2 > 0)
        std::cout << " 0x" << std::hex << (int)data2;

    std::cout << std::endl;
}

void receive(byte* buffer, size_t length)
{
    // Pointers used to search through payload.
    int lPtr = 0;
    int rPtr = 0;

    // lastStatus used to capture runningStatus
    byte lastStatus;

    byte headerByte = buffer[lPtr++];
//    auto signatureIs1 = CHECK_BIT(headerByte, 7 - 1);
//    auto reservedIs0 = !CHECK_BIT(headerByte, 6 - 1);
    auto timestampHigh = 0x3f & headerByte;

    byte timestampByte = buffer[lPtr++];
    uint16_t timestamp = 0;

    bool sysExContinuation = false;

    if (timestampByte >= 80) {
        auto timestampLow = 0x7f & timestampByte;
        timestamp = timestampLow + (timestampHigh << 7);
    }
    else {
        sysExContinuation = true;
        lPtr--; // the second byte is part of the SysEx
    }

    //While statement contains incrementing pointers and breaks when buffer size exceeded.
    while (true)
    {
        lastStatus = buffer[lPtr];

        if ((buffer[lPtr] < 0x80) && !sysExContinuation)
            return; // Status message not present, bail

        // Point to next non-data byte
        rPtr = lPtr;
        while ((buffer[rPtr + 1] < 0x80) && (rPtr < (length - 1)))
            rPtr++;

        // look at l and r pointers and decode by size.
        if (rPtr - lPtr < 1) {
            // Time code or system
            transmitMIDIonDIN(lastStatus, 0, 0);
        }
        else if (rPtr - lPtr < 2) {
            transmitMIDIonDIN(lastStatus, buffer[lPtr + 1], 0);
        }
        else if (rPtr - lPtr < 3) {
            transmitMIDIonDIN(lastStatus, buffer[lPtr + 1], buffer[lPtr + 2]);
        }
        else {
            // Too much data
            // If not System Common or System Real-Time, send it as running status

            auto midiType = buffer[lPtr] & 0xF0;
            if (sysExContinuation)
                midiType = 0xF0;

            switch (midiType)
            {
            case 0x80:
            case 0x90:
            case 0xA0:
            case 0xB0:
            case 0xE0:
                for (auto i = lPtr; i < rPtr; i = i + 2)
                {
                    transmitMIDIonDIN(lastStatus, buffer[i + 1], buffer[i + 2]);
                }
                break;
            case 0xC0:
            case 0xD0:
                for (auto i = lPtr; i < rPtr; i = i + 1)
                {
                    transmitMIDIonDIN(lastStatus, buffer[i + 1], 0);
                }
                break;
            case 0xF0:
                transmitMIDIonDIN(buffer[lPtr], 0, 0);
                for (auto i = lPtr; i < rPtr; i++)
                    transmitMIDIonDIN(buffer[i + 1], 0, 0);

                break;

            default:
                break;
            }
        }

        if (++rPtr >= length)
            return; // end of packet

        timestampByte = buffer[rPtr++];
        if (timestampByte >= 80) // is bit 7 set?
        {
            auto timestampLow = 0x7f & timestampByte;
            timestamp = timestampLow + (timestampHigh << 7);

            std::cout << "timestamp low is 0x" << std::hex << (int)timestampByte << std::endl;
        }

        // Point to next status
        lPtr = rPtr;
        if (lPtr >= length)
            return; //end of packet
    }
}


int main()
{
    std::cout << std::endl << "SysEx with RealTime msg in the middle --------" << std::endl;

    byte sysExAndRealTime[] = { 0xB0, 0xF4,  // header + timestamp
                   0xF0,        // start SysEx
                         0x01, 0x02, 0x03, 0x04, // SysEx data

                   // RealTime message in the middle of a SysEx
                   0xF3, // timestampLow
                   0xFA, // Realtime msg: Start

                         0x05, 0x06, 0x07, 0x08, // rest of sysex data
                   0xF4, // timestampLow
                   0xF7 }; // end of SysEx

    receive(sysExAndRealTime, sizeof(sysExAndRealTime));

    std::cout << std::endl << "SysEx ---------" << std::endl;

    byte sysExPart[] = { 0xB0, 0xF4,  // header + timestamp
                          0xF0, // start SysEx
                            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, // sysex data
                          0xF4, // timestampLow
                          0xF7 }; // end of SysEx

    receive(sysExPart, sizeof(sysExPart));

    std::cout << std::endl << "SysEx part 1" << std::endl;

    byte sysExPart1[] = { 0xB0, 0xF4,  // header + timestamp
                          0xF0, // start SysEx
                            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };  // sysex data
    receive(sysExPart1, sizeof(sysExPart1));

    std::cout << std::endl << "SysEx part 2" << std::endl;

    byte sysExPart2[] = { 0xB0,  // 1 byte header
                          0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, // sysex data (cont)
                          0xF4, // timestampLow
                          0xF7 }; // end of SysEx
    receive(sysExPart2, sizeof(sysExPart2));
    
    std::cout << "ble Packet with 1 MIDI messages" << std::endl;

    byte blePacketWithOneMIDIMessage[] = { 0xA8, 0xC0, 
                                           0x90, 0x3E, 0x3E };
    receive(blePacketWithOneMIDIMessage, sizeof(blePacketWithOneMIDIMessage));
    
    std::cout << std::endl << "ble Packet with 2 MIDI messages" << std::endl;

    byte blePacketWithTwoMIDIMessage[] = { 0xA8, 0xC0,
                                           0x90, 0x3E, 0x3E,
                                           0xC1,
                                           0x91, 0x3E, 0x3E };
    receive(blePacketWithTwoMIDIMessage, sizeof(blePacketWithTwoMIDIMessage));

    std::cout << std::endl << "ble Packet with 3 MIDI messages" << std::endl;

    byte blePacketWithThreeMIDIMessage[] = { 0xA8, 0xC0,
                                           0x90, 0x3E, 0x3E,
                                           0xC1,
                                           0xF0,
                                            0x01, 0x02,
                                           0xC2,
                                           0xF7,
                                           0xC3,
                                           0x91, 0x3E, 0x3E };
    receive(blePacketWithThreeMIDIMessage, sizeof(blePacketWithThreeMIDIMessage));


//    std::cout << std::endl << "2 MIDI messages with running status" << std::endl;

// TODO
    
//    byte twoMIDIMessageWithRunningStatus[] = { 0xA9, 0xAE, 0xD1, 0x74, 0xAF, 0xD2, 0x74, 0xB1 };
//    receive(twoMIDIMessageWithRunningStatus, sizeof(twoMIDIMessageWithRunningStatus));

//    std::cout << std::endl << "2 MIDI messages with running status" << std::endl;
//
//    byte multipleMIDIMessagesMixedType[] = { 0x00 };
//    receive(multipleMIDIMessagesMixedType, sizeof(multipleMIDIMessagesMixedType));

}