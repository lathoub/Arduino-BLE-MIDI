#pragma once

// Headers for ESP32 NimBLE
#include <NimBLEDevice.h>

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI_ESP32_NimBLE
{
private:
    BLEServer*			_server = nullptr;
    BLEAdvertising*		_advertising = nullptr;
    BLECharacteristic*	_characteristic = nullptr;
        
    BLEMIDITransport<class BLEMIDI_ESP32_NimBLE>* _bleMidiTransport = nullptr;

    friend class MyServerCallbacks; 
    friend class MyCharacteristicCallbacks; 

protected:
    QueueHandle_t mRxQueue;

public:
	BLEMIDI_ESP32_NimBLE()
    {
    }
    
	bool begin(const char*, BLEMIDITransport<class BLEMIDI_ESP32_NimBLE>*);
    
    void write(uint8_t* buffer, size_t length)
    {
        _characteristic->setValue(buffer, length);
        _characteristic->notify();
    }
    
    size_t available(void *pvBuffer)
    {
        return xQueueReceive(mRxQueue, pvBuffer, 0); // return immediately when the queue is empty
    }

    void add(const void* value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        xQueueSend(mRxQueue, value, portMAX_DELAY);
    }

protected:
	void receive(uint8_t* buffer, size_t length)
	{
        // parse the incoming buffer
        _bleMidiTransport->receive(buffer, length);
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
    MyServerCallbacks(BLEMIDI_ESP32_NimBLE* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32) {
    }

protected:
	BLEMIDI_ESP32_NimBLE* _bluetoothEsp32 = nullptr;

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
    MyCharacteristicCallbacks(BLEMIDI_ESP32_NimBLE* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32 ) {
    }
    
protected:
	BLEMIDI_ESP32_NimBLE* _bluetoothEsp32 = nullptr;

    void onWrite(BLECharacteristic * characteristic) {
        std::string rxValue = characteristic->getValue();
        if (rxValue.length() > 0) {
			_bluetoothEsp32->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
        }
    }
};

bool BLEMIDI_ESP32_NimBLE::begin(const char* deviceName, BLEMIDITransport<class BLEMIDI_ESP32_NimBLE>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    BLEDevice::init(deviceName);
    
    // To communicate between the 2 cores.
    // Core_0 runs here, core_1 runs the BLE stack
    mRxQueue = xQueueCreate(64, sizeof(uint8_t)); // TODO Settings::MaxBufferSize

    _server = BLEDevice::createServer();
    _server->setCallbacks(new MyServerCallbacks(this));
    
    // Create the BLE Service
    auto service = _server->createService(BLEUUID(SERVICE_UUID));
    
    // Create a BLE Characteristic
    _characteristic = service->createCharacteristic(
                                                     BLEUUID(CHARACTERISTIC_UUID),
                                                     NIMBLE_PROPERTY::READ   |
                                                     NIMBLE_PROPERTY::WRITE  |
                                                     NIMBLE_PROPERTY::NOTIFY |
                                                     NIMBLE_PROPERTY::WRITE_NR
                                                     );

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

 /*! \brief Create an instance for ESP32 named <DeviceName>
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> BLE##Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE>, MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI_ESP32_NimBLE> &)BLE##Name);

 /*! \brief Create a default instance for ESP32 named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
BLEMIDI_CREATE_INSTANCE("BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
