#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BEGIN_BLEMIDI_NAMESPACE

class BleMidiInterface;

class MyServerCallbacks: public BLEServerCallbacks {
public:
    MyServerCallbacks(BleMidiInterface* bleMidiInterface) {
        _bleMidiInterface = bleMidiInterface;
    }
protected:
    BleMidiInterface* _bleMidiInterface;

    void onConnect(BLEServer* pServer) {
      //  _bleMidiInterface->mConnectedCallback();
    };
    
    void onDisconnect(BLEServer* pServer) {
      //  _bleMidiInterface->mDisconnectedCallback();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
    }
};

class BleMidiInterface
{
protected:
    // ESP32
    BLEServer  *pServer;
    BLEAdvertising *pAdvertising;
    BLECharacteristic *pCharacteristic;

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

    inline bool begin(const char* deviceName)
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
    
    inline void onConnected(void(*fptr)()) {
        mConnectedCallback = fptr;
    }
    inline void onDisconnected(void(*fptr)()) {
        mDisconnectedCallback = fptr;
    }
};

END_BLEMIDI_NAMESPACE
