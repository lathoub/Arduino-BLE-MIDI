#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
//#include "BLEScan.h"
#include "Arduino.h"

#include "common/midiCommon.h"
using namespace Midi;

BEGIN_BLEMIDI_NAMESPACE

static bool isDataReceived = false;
static bool doScan = false;
static  BLEScan* pBLEScan;   

static void notifyCallback(
        BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify) 
{
    isDataReceived = true;
}

static void scanCompleteCB(BLEScanResults scanResults) 
    {
        scanResults.dump();
        if (doScan)
             pBLEScan->start(10, scanCompleteCB);
        {   
        }
    }
  
class MyClientCallbacks;

class BleMidiInterfaceClient : public MidiCommonInterface
{
protected:
    // ESP32
    BLEClient  * pClient;
    //BLEAdvertising * _advertising;
    //BLECharacteristic *_characteristic;
 
    uint8_t _midiPacket[5]; // outgoing
    
public:

    void * operator new(size_t size)                             
    {                                                            
        void * p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);   
        return p;                                               
    }      
    
    // callbacks
    void(*_connectedCallback)() = NULL;
    void(*_disconnectedCallback)() = NULL;
    char _deviceName[32];
    bool _doConnect;
    BLEAdvertisedDevice * _advertising = NULL;
    BLERemoteService* pRemoteService;
    BLERemoteCharacteristic* pRemoteCharacteristic;
    bool _connected;
    
protected:
    inline static void getMidiTimestamp (uint8_t *header, uint8_t *timestamp)
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
    
    // serialize towards hardware
    
    void write(DataByte b1)
    {
        getMidiTimestamp(&_midiPacket[0], &_midiPacket[1]);
 
        _midiPacket[2] = b1;

        // TODO: quid running status

        if(pRemoteCharacteristic && pRemoteCharacteristic->canWrite())
        {
            pRemoteCharacteristic->writeValue(_midiPacket, 3);
        }
    };
    
    void write(DataByte b1, DataByte b2)
    {
        getMidiTimestamp(&_midiPacket[0], &_midiPacket[1]);

        _midiPacket[2] = b1;
        _midiPacket[3] = b2;

        // TODO: quid running status
        if(pRemoteCharacteristic && pRemoteCharacteristic->canWrite())
        {
            pRemoteCharacteristic->writeValue(_midiPacket, 4);
        }
    };
    
    void write(DataByte b1, DataByte b2, DataByte b3)
    {
        getMidiTimestamp(&_midiPacket[0], &_midiPacket[1]);
        
        _midiPacket[2] = b1;
        _midiPacket[3] = b2;
        _midiPacket[4] = b3;

        // TODO: quid running status
         if(pRemoteCharacteristic && pRemoteCharacteristic->canWrite())
        {
            pRemoteCharacteristic->writeValue(_midiPacket, 5);
        }
   };

   bool connectToServer() 
    {
        const uint8_t bothOn[] = {0x3, 0x0};
        // Connect to the remote BLE Server.
        pClient->connect(_advertising);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    
        // Obtain a reference to the service we are after in the remote BLE server.
        pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
        if (pRemoteService == nullptr) 
        {
            pClient->disconnect();
            return false;
        }
        
        // Obtain a reference to the characteristic in the service of the remote BLE server.
        pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
        if (pRemoteCharacteristic == nullptr) 
        {
            pClient->disconnect();
            return false;
        }

        pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)bothOn, 2, true);
        return true;
    }; 


public:
    BleMidiInterfaceClient()
    {
    }
    
     ~BleMidiInterfaceClient()
    {
    }
    
    // TODO why must these functions be inline??
    
    inline bool begin(const char* thisDeviceName, const char *remoteDeviceName);
    
    inline void read()
    {
        // If the flag "doConnect" is true then we have scanned for and found the desired
        // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
        // connected we set the connected flag to be true.
        if (_doConnect == true) 
        {
            if (connectToServer())
            {
            // ("We are now connected to the BLE Server.");
                if (nullptr != pRemoteCharacteristic)
                {
                    pRemoteCharacteristic->registerForNotify(notifyCallback);
                }
            }
            else 
            {
                // ("We have failed to connect to the server; there is nothin more we will do.");
                doScan = true;
                pBLEScan->start(10, scanCompleteCB);
               
            }

             _doConnect = false;
        }
        

       if (false == _connected)
       {
           return;
       }


       if (nullptr == pRemoteCharacteristic)
       {
           return;
       }
		   // Read the value of the characteristic.
       //if(pRemoteCharacteristic->canRead())
        if (isDataReceived)
        {
            std::string rxValue = pRemoteCharacteristic->readValue();
            if (rxValue.length() > 0)
            {
                this->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
            }

            isDataReceived = false;
        } 
    }
    
    inline void sendMIDI(StatusByte, DataByte data1 = 0, DataByte data2 = 0);
    inline void receive(uint8_t *buffer, uint8_t bufferSize);

    void onConnected(void(*fptr)()) 
    {
        _connectedCallback = fptr;
    }
    void onDisconnected(void(*fptr)()) 
    {
        _disconnectedCallback = fptr;
    } 
};

class MyClientCallbacks: public BLEClientCallbacks {
public:
    MyClientCallbacks(BleMidiInterfaceClient* BleMidiInterfaceClient) {
        _BleMidiInterfaceClient = BleMidiInterfaceClient;
    }
protected:
    BleMidiInterfaceClient* _BleMidiInterfaceClient;

    void onConnect(BLEClient* client) 
    {
        if (_BleMidiInterfaceClient->_connectedCallback)
        {
             _BleMidiInterfaceClient->_connectedCallback();
        }
         _BleMidiInterfaceClient->_connected = true;
    };
    
    void onDisconnect(BLEClient* client) 
    {
        if (_BleMidiInterfaceClient->_disconnectedCallback)
        {
            _BleMidiInterfaceClient->_disconnectedCallback();
        }

        _BleMidiInterfaceClient->_doConnect = false;
        _BleMidiInterfaceClient->pRemoteService = nullptr;
        _BleMidiInterfaceClient->pRemoteCharacteristic = nullptr; 
        doScan = true;
        pBLEScan->start(10, scanCompleteCB);
    }

};


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  public:

    MyAdvertisedDeviceCallbacks(BleMidiInterfaceClient* BleMidiInterfaceClient) : BLEAdvertisedDeviceCallbacks()
    {
        _BleMidiInterfaceClient = BleMidiInterfaceClient;
    }

  void onResult(BLEAdvertisedDevice advertisedDevice) {

    if (strstr(advertisedDevice.toString().c_str(), _BleMidiInterfaceClient->_deviceName) == NULL)
    {
      // Check that the device matches our host name
      return;
    } 


    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID)))
     {
      
      BLEDevice::getScan()->stop();

      if (_BleMidiInterfaceClient->_advertising != NULL)
      {
          delete(_BleMidiInterfaceClient->_advertising);
          _BleMidiInterfaceClient->_advertising = NULL;
      }
      _BleMidiInterfaceClient->_advertising = new BLEAdvertisedDevice(advertisedDevice);
      _BleMidiInterfaceClient->_doConnect = true;
      doScan = false;

    } // Found our server
  } // onResult

protected:
    BleMidiInterfaceClient* _BleMidiInterfaceClient;
 
}; // MyAdvertisedDeviceCallbacks

bool BleMidiInterfaceClient::begin(const char* thisDeviceName, const char *remoteDeviceName)
{
    BLEDevice::init(thisDeviceName);
    
    strncpy(_deviceName, remoteDeviceName, 32);

    pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks(this));

     // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(this));
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    doScan = true;
    pBLEScan->start(10, scanCompleteCB);
    
   
    
    return true;
}


void BleMidiInterfaceClient::sendMIDI(StatusByte status, DataByte data1, DataByte data2)
{
    MidiType type   = getTypeFromStatusByte(status);
    Channel channel = getChannelFromStatusByte(status);

    switch (type) {
        case NoteOff:
            if (_noteOffCallback) _noteOffCallback(channel, data1, data2);
            break;
        case NoteOn:
            if (_noteOnCallback) _noteOnCallback(channel, data1, data2);
            break;
        case AfterTouchPoly:
            if (_afterTouchPolyCallback) _afterTouchPolyCallback(channel, data1, data2);
            break;
        case ControlChange:
            if (_controlChangeCallback) _controlChangeCallback(channel, data1, data2);
            break;
        case ProgramChange:
            if (_programChangeCallback) _programChangeCallback(channel, data1);
            break;
        case AfterTouchChannel:
            if (_afterTouchChannelCallback) _afterTouchChannelCallback(channel, data1);
            break;
        case PitchBend:
            if (_pitchBendCallback) {
                int value = (int) ((data1 & 0x7f) | ((data2 & 0x7f) << 7)) + MIDI_PITCHBEND_MIN;
                _pitchBendCallback(channel, value);
            }
            break;
            
        case SystemExclusive:
            break;
            
        case TimeCodeQuarterFrame:
            if (_timeCodeQuarterFrameCallback) _timeCodeQuarterFrameCallback(data1);
            break;
        case SongPosition:
            if (_songPositionCallback) {
                unsigned short value = unsigned((data1 & 0x7f) | ((data2 & 0x7f) << 7));
                _songPositionCallback(value);
            }
            break;
        case SongSelect:
            if (_songSelectCallback) _songSelectCallback(data1);
            break;
        case TuneRequest:
            if (_tuneRequestCallback) _tuneRequestCallback();
            break;
            
        case Clock:
            if (_clockCallback) _clockCallback();
            break;
        case Tick:
            break;
        case Start:
            if (_startCallback) _startCallback();
            break;
        case Continue:
            if (_continueCallback) _continueCallback();
            break;
        case Stop:
            if (_stopCallback) _stopCallback();
            break;
        case ActiveSensing:
            if (_activeSensingCallback) _activeSensingCallback();
            break;
        case SystemReset:
            if (_resetCallback) _resetCallback();
            break;
    }
}

void BleMidiInterfaceClient::receive(uint8_t *buffer, uint8_t bufferSize)
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
    
    //Pointers used to search through payload.
    uint8_t lPtr = 0;
    uint8_t rPtr = 0;
    
    //lastStatus used to capture runningStatus
    uint8_t lastStatus;
    
    //Decode first packet -- SHALL be "Full MIDI message"
    lPtr = 2; //Start at first MIDI status -- SHALL be "MIDI status"
    
    //While statement contains incrementing pointers and breaks when buffer size exceeded.
    while(1){
        lastStatus = buffer[lPtr];
        if( (buffer[lPtr] < 0x80) ){
            //Status message not present, bail
            return;
        }
        //Point to next non-data byte
        rPtr = lPtr;
        while( (buffer[rPtr + 1] < 0x80)&&(rPtr < (bufferSize - 1)) ){
            rPtr++;
        }
        //look at l and r pointers and decode by size.
        if( rPtr - lPtr < 1 ){
            //Time code or system
            sendMIDI(lastStatus);
        } else if( rPtr - lPtr < 2 ) {
            sendMIDI(lastStatus, buffer[lPtr + 1]);
        } else if( rPtr - lPtr < 3 ) {
            sendMIDI(lastStatus, buffer[lPtr + 1], buffer[lPtr + 2]);
        } else {
            //Too much data
            //If not System Common or System Real-Time, send it as running status
            switch( buffer[lPtr] & 0xF0 )
            {
                case NoteOff:
                case NoteOn:
                case AfterTouchPoly:
                case ControlChange:
                case PitchBend:
                    for(int i = lPtr; i < rPtr; i = i + 2)
                        sendMIDI(lastStatus, buffer[i + 1], buffer[i + 2]);
                    break;
                case ProgramChange:
                case AfterTouchChannel:
                    for(int i = lPtr; i < rPtr; i = i + 1)
                        sendMIDI(lastStatus, buffer[i + 1]);
                    break;
                default:
                    break;
            }
        }
        //Point to next status
        lPtr = rPtr + 2;
        if(lPtr >= bufferSize){
            //end of packet
            return;
        }
    }

}

END_BLEMIDI_NAMESPACE
