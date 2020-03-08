#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI_ESP32
{
private:
    BLEServer*			_server = nullptr;
    BLEAdvertising*		_advertising = nullptr;
    BLECharacteristic*	_characteristic = nullptr;
        
	BLEMIDI<class BLEMIDI_ESP32>* _bleMidiTransport = nullptr;

public:
	BLEMIDI_ESP32()
    {
    }
    
	bool begin(const char*, BLEMIDI<class BLEMIDI_ESP32>*);
    
    void write(uint8_t* data, uint8_t length)
    {
        _characteristic->setValue(data, length);
        _characteristic->notify();
    }
    
	void receive(uint8_t* buffer, size_t length)
	{
        // Post the items to the back of the queue
        // (drop the first 2 items)
        for (size_t i = 2; i < length; i++)
            xQueueSend(_bleMidiTransport->mRxQueue, &buffer[i], portMAX_DELAY);
	}

	void connected()
	{
		if (_bleMidiTransport->_connectedCallback)
			_bleMidiTransport->_connectedCallback();
	}

	void disconnected()
	{
		if (_bleMidiTransport->_disconnectedCallback)
			_bleMidiTransport->_disconnectedCallback();
	}
};

class MyServerCallbacks: public BLEServerCallbacks {
public:
    MyServerCallbacks(BLEMIDI_ESP32* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32) {
    }

protected:
	BLEMIDI_ESP32* _bluetoothEsp32 = nullptr;

    void onConnect(BLEServer*) {
        if (_bluetoothEsp32)
            _bluetoothEsp32->connected();
    };
    
    void onDisconnect(BLEServer*) {
        if (_bluetoothEsp32)
            _bluetoothEsp32->disconnected();
	}
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
public:
    MyCharacteristicCallbacks(BLEMIDI_ESP32* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32 ) {
    }
    
protected:
	BLEMIDI_ESP32* _bluetoothEsp32 = nullptr;

    void onWrite(BLECharacteristic * characteristic) {
        std::string rxValue = characteristic->getValue();
        if (rxValue.length() > 0) {
			_bluetoothEsp32->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
        }
    }
};

bool BLEMIDI_ESP32::begin(const char* deviceName, BLEMIDI<class BLEMIDI_ESP32>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    BLEDevice::init(deviceName);
    
    _server = BLEDevice::createServer();
    _server->setCallbacks(new MyServerCallbacks(this));
    
    // Create the BLE Service
    auto service = _server->createService(BLEUUID(SERVICE_UUID));
    
    // Create a BLE Characteristic
    _characteristic = service->createCharacteristic(
                                                     BLEUUID(CHARACTERISTIC_UUID),
                                                     BLECharacteristic::PROPERTY_READ   |
                                                     BLECharacteristic::PROPERTY_WRITE  |
                                                     BLECharacteristic::PROPERTY_NOTIFY |
                                                     BLECharacteristic::PROPERTY_WRITE_NR
                                                     );
    // Add CCCD 0x2902 to allow notify
    _characteristic->addDescriptor(new BLE2902());

    _characteristic->setCallbacks(new MyCharacteristicCallbacks(this));
    // Start the service
    service->start();
    
    auto advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x04);
    advertisementData.setCompleteServices(BLEUUID(SERVICE_UUID));
    advertisementData.setName(deviceName);

    // Start advertising
    _advertising = _server->getAdvertising();
    _advertising->setAdvertisementData(advertisementData);
    _advertising->start();
    
    return true;
}

END_BLEMIDI_NAMESPACE
