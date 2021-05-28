#pragma once

// Headers for ESP32 BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI_Client_ESP32
{
private:
    BLEClient*			_client = nullptr;
        
	BLEMIDI<class BLEMIDI_Client_ESP32>* _bleMidiTransport = nullptr;

public:
	BLEMIDI_Client_ESP32()
    {
    }
    
	bool begin(const char*, BLEMIDI<class BLEMIDI_Client_ESP32>*);

    void end() 
    {
        
    }
        
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

class MyClientCallbacks: public BLEClientCallbacks {
public:
    MyClientCallbacks(BLEMIDI_Client_ESP32* bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32) {
    }

protected:
	BLEMIDI_Client_ESP32* _bluetoothEsp32 = nullptr;

    void onConnect(BLEClient*) {
        if (_bluetoothEsp32)
            _bluetoothEsp32->connected();
    };
    
    void onDisconnect(BLEClient*) {
        if (_bluetoothEsp32)
            _bluetoothEsp32->disconnected();
	}
};

bool BLEMIDI_Client_ESP32::begin(const char* deviceName, BLEMIDI<class BLEMIDI_ESP32>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    BLEDevice::init(deviceName);
    
    _client = BLEDevice::createClient();
    _client->setCallbacks(new MyClientCallbacks(this));
    
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

END_BLEMIDI_NAMESPACE
