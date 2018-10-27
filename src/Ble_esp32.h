#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

BEGIN_BLEMIDI_NAMESPACE

class BleMidiInterface
{
protected:
    // ESP32
    BLEServer  *pServer;
    BLEAdvertising *pAdvertising;
    BLECharacteristic *pCharacteristic;
    
    bool _connected;
    
public:
    void(*mConnectedCallback)();
    void(*mDisconnectedCallback)();
    
public:
    inline BleMidiInterface()
    {
        mConnectedCallback = NULL;
        mDisconnectedCallback = NULL;
    }
    
    inline ~BleMidiInterface()
    {
    }
    
    inline bool begin(const char* deviceName);
    
    inline void noteOn(DataByte note, DataByte velocity, Channel channel) {
        
        if (!_connected) return;
        if (pCharacteristic == NULL) return;

        uint8_t midiPacket[] = {
            0x80,  // header
            0x80,  // timestamp, not implemented
            0x00,  // status
            0x3c,  // 0x3c == 60 == middle c
            0x00   // velocity
        };
        
        midiPacket[2] = note;      // note, channel 0
        midiPacket[4] = velocity;  // velocity
        pCharacteristic->setValue(midiPacket, 5); // packet, length in bytes
        pCharacteristic->notify();
    }

    inline void noteOff(DataByte note, DataByte velocity, Channel channel) {
    }
    
    inline void programChange(DataByte inProgramNumber, Channel inChannel) {
        
    }
    inline void controlChange(DataByte inControlNumber, DataByte inControlValue, Channel inChannel) {
        
    }
    inline void pitchBend(int inPitchValue, Channel inChannel) {
        
    }
    inline void pitchBend(double inPitchValue, Channel inChannel) {
        
    }
    inline void polyPressure(DataByte inNoteNumber, DataByte inPressure, Channel inChannel) {
        
    }
    inline void afterTouch(DataByte inPressure, Channel inChannel) {
        
    }
    inline void sysEx(const byte*, uint16_t inLength) {
        
    }
    inline void timeCodeQuarterFrame(DataByte inTypeNibble, DataByte inValuesNibble) {
        
    }
    inline void timeCodeQuarterFrame(DataByte inData) {
        
    }
    inline void songPosition(unsigned short inBeats) {
        
    }
    inline void songSelect(DataByte inSongNumber) {
        
    }
    inline void tuneRequest() {
        
    }
    inline void activeSensing() {
        
    }
    inline void start() {
        
    }
    inline void _continue() {
        
    }
    inline void stop() {
        
    }
    inline void reset() {
        
    }
    inline void clock() {
        
    }
    inline void tick() {
        
    }

    inline void onConnected(void(*fptr)()) {
        _connected = true;
        mConnectedCallback = fptr;
    }
    inline void onDisconnected(void(*fptr)()) {
        _connected = false;
        mDisconnectedCallback = fptr;
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
        _bleMidiInterface->mConnectedCallback();
    };
    
    void onDisconnect(BLEServer* pServer) {
        _bleMidiInterface->mDisconnectedCallback();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
    }
};

bool BleMidiInterface::begin(const char* deviceName)
{
    BLEDevice::init(deviceName);
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(this));
    
    // Create the BLE Service
    BLEService* pService = pServer->createService(BLEUUID(SERVICE_UUID));
    
    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                                                     BLEUUID(CHARACTERISTIC_UUID),
                                                     BLECharacteristic::PROPERTY_READ   |
                                                     BLECharacteristic::PROPERTY_WRITE  |
                                                     BLECharacteristic::PROPERTY_NOTIFY |
                                                     BLECharacteristic::PROPERTY_WRITE_NR
                                                     );
    pCharacteristic->setCallbacks(new MyCallbacks());
    // Start the service
    pService->start();
    
    BLEAdvertisementData advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x04);
    advertisementData.setCompleteServices(BLEUUID(SERVICE_UUID));
    advertisementData.setName(deviceName);
    
    // Start advertising
    pAdvertising = pServer->getAdvertising();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    
    return true;
}

END_BLEMIDI_NAMESPACE
