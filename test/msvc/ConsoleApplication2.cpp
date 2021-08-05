// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

typedef unsigned int word;
typedef uint8_t boolean;
typedef unsigned char byte;

#define CHECK_BIT(var,pos) (!!((var) & (1<<(pos))))

//#define RUNNINGSTATUS_ENABLE

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
	// previousStatus used to continue a runningStatus interrupted by a timeStamp or a System Message.
	byte previousStatus = 0x00;
	

    byte headerByte = buffer[lPtr++];
//    auto signatureIs1 = CHECK_BIT(headerByte, 7 - 1);
//    auto reservedIs0 = !CHECK_BIT(headerByte, 6 - 1);
    auto timestampHigh = 0x3f & headerByte;

    byte timestampByte = buffer[lPtr++];
    uint16_t timestamp = 0;

    bool sysExContinuation = false;
	bool runningStatusContinuation = false;
	

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
		
		if(previousStatus==0x00){
			if ((lastStatus < 0x80) && !sysExContinuation)
				return; // Status message not present and it is not a runningStatus continuation, bail
		}else if(lastStatus < 0x80)
		{
			lastStatus = previousStatus;
			runningStatusContinuation = true;
		}
			

        // Point to next non-data byte
        rPtr = lPtr;
        while ((buffer[rPtr + 1] < 0x80) && (rPtr < (length - 1)))
            rPtr++;

		if(!runningStatusContinuation){
			// If not System Common or System Real-Time, send it as running status

			auto midiType = lastStatus & 0xF0;
			if (sysExContinuation)
				midiType = 0xF0;

			switch (midiType)
			{
			
			// 3 bytes
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xE0:
#ifndef RUNNINGSTATUS_ENABLE
				for (auto i = lPtr; i < rPtr; i = i + 2)
				{

					transmitMIDIonDIN(lastStatus, buffer[i + 1], buffer[i + 2]);
				}
#else
				transmitMIDIonDIN(lastStatus, 0, 0);
				for (auto i = lPtr; i < rPtr; i = i + 2)
				{

					transmitMIDIonDIN(buffer[i + 1], buffer[i + 2], 0);
				}
#endif
				break;
				
			// 2 bytes
			case 0xC0:
			case 0xD0:
#ifndef RUNNINGSTATUS_ENABLE
				for (auto i = lPtr; i < rPtr; i = i + 1)
				{
					transmitMIDIonDIN(lastStatus, buffer[i + 1], 0);
				}
#else
				transmitMIDIonDIN(lastStatus, 0, 0);
				for (auto i = lPtr; i < rPtr; i = i + 1)
				{

					transmitMIDIonDIN(buffer[i + 1], 0 , 0);
				}
#endif
				break;
			// 1 byte or n bytes
			case 0xF0:
				transmitMIDIonDIN(buffer[lPtr], 0, 0);
				for (auto i = lPtr; i < rPtr; i++)
					transmitMIDIonDIN(buffer[i + 1], 0, 0);

				break;

			default:
				break;
			}
		}
		else
		{
			
#ifndef RUNNINGSTATUS_ENABLE
			auto midiType = lastStatus & 0xF0;
            if (sysExContinuation)
                midiType = 0xF0;

            switch (midiType)
            {
            case 0x80:
            case 0x90:
            case 0xA0:
            case 0xB0:
            case 0xE0:
				//3 bytes full Midi -> 2 bytes runningStatus
                for (auto i = lPtr; i <= rPtr; i = i + 2)
                {
                    transmitMIDIonDIN(lastStatus, buffer[i], buffer[i + 1]);
                }
                break;
            case 0xC0:
            case 0xD0:
				//2 bytes full Midi -> 1 byte runningStatus
                for (auto i = lPtr; i <= rPtr; i = i + 1)
                {
                    transmitMIDIonDIN(lastStatus, buffer[i], 0);
                }
                break;

            default:
                break;
            }
#else					
			transmitMIDIonDIN(lastStatus, 0, 0);
			
			for (auto i = lPtr; i <= rPtr; i++)
            {
                transmitMIDIonDIN(buffer[i], 0, 0);
            }
#endif			
			runningStatusContinuation = false;
		}

        if (++rPtr >= length)
            return; // end of packet
		
		if(lastStatus < 0xf0) //exclude System Message. They must not be RunningStatus
		{ 
			previousStatus = lastStatus;
		}

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


    std::cout << std::endl << "2 MIDI messages with multiple running status" << std::endl;
 
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
    receive(twoMIDIMessageWithRunningStatus, sizeof(twoMIDIMessageWithRunningStatus));


    std::cout << std::endl << "2 MIDI messages with multiple running status and a System message in middle" << std::endl;
 
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
    receive(twoMIDIMessageWithRunningStatusPlusSys, sizeof(twoMIDIMessageWithRunningStatusPlusSys));
}
