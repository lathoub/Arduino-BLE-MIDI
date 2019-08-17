#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BEGIN_BLEMIDI_NAMESPACE

class BluetoothEsp32
{
private:
    BLEServer*			_server;
    BLEAdvertising*		_advertising;
    BLECharacteristic*	_characteristic;
        
	BleMidiTransport<class BluetoothEsp32>* _bleMidiTransport;

public:
	BluetoothEsp32()
		:   _server(NULL),
            _advertising(NULL),
            _characteristic(NULL),
            _bleMidiTransport(NULL)
    {
    }
    
	~BluetoothEsp32()
	{
	}

	bool begin(const char*, BleMidiTransport<class BluetoothEsp32>*);
    
    inline void write(uint8_t* data, uint8_t length)
    {
        _characteristic->setValue(data, length);
        _characteristic->notify();
    }
    
	inline void receive(uint8_t* buffer, uint8_t length)
	{
		_bleMidiTransport->receive(buffer, length);
	}

	inline void connected()
	{
		if (_bleMidiTransport->_connectedCallback)
			_bleMidiTransport->_connectedCallback();
	}

	inline void disconnected()
	{
		if (_bleMidiTransport->_disconnectedCallback)
			_bleMidiTransport->_disconnectedCallback();
	}
};

class MyServerCallbacks: public BLEServerCallbacks {
public:
    MyServerCallbacks(BluetoothEsp32* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32) {
    }

protected:
	BluetoothEsp32* _bluetoothEsp32;

    void onConnect(BLEServer* server) {
		_bluetoothEsp32->connected();
    };
    
    void onDisconnect(BLEServer* server) {
		_bluetoothEsp32->disconnected();
	}
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
public:
    MyCharacteristicCallbacks(BluetoothEsp32* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32 ) {
    }
    
protected:
	BluetoothEsp32* _bluetoothEsp32;

    void onWrite(BLECharacteristic * characteristic) {
        std::string rxValue = characteristic->getValue();
        if (rxValue.length() > 0) {
			_bluetoothEsp32->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
        }
    }
};

bool BluetoothEsp32::begin(const char* deviceName, BleMidiTransport<class BluetoothEsp32>* bleMidiTransport)
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
