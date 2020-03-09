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
    
    void write(uint8_t* buffer, size_t length)
    {
        _characteristic->setValue(buffer, length);
        _characteristic->notify();
    }
    
	void receive(uint8_t* buffer, size_t length)
	{
        // Pointers used to search through payload.
        uint8_t lPtr = 0;
        uint8_t rPtr = 0;
        // lastStatus used to capture runningStatus
        uint8_t lastStatus;
        // Decode first packet -- SHALL be "Full MIDI message"
        lPtr = 2; //Start at first MIDI status -- SHALL be "MIDI status"
        
        //While statement contains incrementing pointers and breaks when buffer size exceeded.
        while (true)
        {
            lastStatus = buffer[lPtr];
            
            if( (buffer[lPtr] < 0x80))
                return; // Status message not present, bail

            // Point to next non-data byte
            rPtr = lPtr;
            while( (buffer[rPtr + 1] < 0x80) && (rPtr < (length - 1)) )
                rPtr++;
            if (buffer[rPtr + 1] == 0xF7) rPtr++;

            // look at l and r pointers and decode by size.
            if( rPtr - lPtr < 1 ) {
                // Time code or system
                xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr], portMAX_DELAY);
            } else if( rPtr - lPtr < 2 ) {
                 xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr], portMAX_DELAY);
                 xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr + 1], portMAX_DELAY);
            } else if( rPtr - lPtr < 3 ) {
                 xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr], portMAX_DELAY);
                 xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr + 1], portMAX_DELAY);
                 xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr + 2], portMAX_DELAY);
            } else {
                // Too much data
                // If not System Common or System Real-Time, send it as running status
                switch(buffer[lPtr] & 0xF0)
                {
                case 0x80:
                case 0x90:
                case 0xA0:
                case 0xB0:
                case 0xE0:
             //       for (auto i = lPtr; i < rPtr; i = i + 2)
               //         transmitMIDIonDIN( lastStatus, buffer[i + 1], buffer[i + 2] );
                    break;
                case 0xC0:
                case 0xD0:
                 //   for (auto i = lPtr; i < rPtr; i = i + 1)
                   //     transmitMIDIonDIN( lastStatus, buffer[i + 1], 0 );
                    break;
                case 0xF0:
                    xQueueSend(_bleMidiTransport->mRxQueue, &buffer[lPtr], portMAX_DELAY);
                    for (auto i = lPtr; i < rPtr; i++)
                        xQueueSend(_bleMidiTransport->mRxQueue, &buffer[i + 1], portMAX_DELAY);
                    break;
                default:
                    break;
                }
            }
            
            // Point to next status
            lPtr = rPtr + 2;
            if(lPtr >= length)
                return; //end of packet
        }
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
