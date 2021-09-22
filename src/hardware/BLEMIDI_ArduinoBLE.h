#pragma once

#include <ArduinoBLE.h>

BLEService midiService(SERVICE_UUID);
BLEStringCharacteristic midiChar(CHARACTERISTIC_UUID,  // standard 16-bit characteristic UUID
    BLERead | BLEWrite | BLENotify | BLEWriteWithoutResponse, 16); // remote clients will be able to get notifications if this characteristic changes

#define BLE_POLLING

BEGIN_BLEMIDI_NAMESPACE

template<typename T, int rawSize>
class Fifo {
public:
	const size_t size;				//speculative feature, in case it's needed

	Fifo(): size(rawSize) 
    {
	    flush();
    }

	T dequeue()
    {
        numberOfElements--;
        nextOut %= size;
        return raw[ nextOut++];
    };
	
    bool enqueue( T element )
    {
        if ( count() >= rawSize )
            return false;

        numberOfElements++;
        nextIn %= size;
        raw[nextIn] = element;
        nextIn++; //advance to next index

        return true;
    };

    T peek() const
    {
	    return raw[ nextOut % size];
    }

    void flush()
    {
	    nextIn = nextOut = numberOfElements = 0;
    }

	// how many elements are currently in the FIFO?
	size_t count() { return numberOfElements; }

private:
    size_t numberOfElements;
    size_t nextIn;
    size_t nextOut;
    T raw[rawSize];
};

class BLEMIDI_ArduinoBLE
{
private:   
    static BLEMIDI_Transport<class BLEMIDI_ArduinoBLE>* _bleMidiTransport;
    static BLEDevice* _central;

    Fifo<byte, 64> mRxBuffer;

public:
	BLEMIDI_ArduinoBLE()
    {
    }
    
	bool begin(const char*, BLEMIDI_Transport<class BLEMIDI_ArduinoBLE>*);
    
    void end() 
    {
        
    }

    void write(uint8_t* buffer, size_t length)
    {
        // TODO: test length
        ((BLECharacteristic)midiChar).writeValue(buffer, length);
    }
    
    bool available(byte* pvBuffer)
    {
 #ifdef BLE_POLLING

        if (mRxBuffer.count() > 0) {
            *pvBuffer = mRxBuffer.dequeue();

            return true;
        }

        poll();
    
        if (midiChar.written()) {
          //  auto buffer = midiChar.value();
            auto length = midiChar.valueLength();

            if (length > 0) {
                auto buffer = midiChar.value().c_str();
                _bleMidiTransport->receive((byte*)buffer, length);
 
            }
        }
        return false;
#endif
#ifdef BLE_EVENTS
/      BLE.poll();
        return ; // ??
#endif
    }

    void add(byte value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        mRxBuffer.enqueue(value);
    }

protected:
	static void receive(const unsigned char* buffer, size_t length)
	{
        // forward the buffer so it can be parsed
        _bleMidiTransport->receive((uint8_t*)buffer, length);
	}

	bool poll()
	{
        BLEDevice central = BLE.central();
        if (!central) {
            if (_central) {
                BLEMIDI_ArduinoBLE::blePeripheralDisconnectHandler(*_central);
                _central = nullptr;
            }
            return false;
        }

        if (!central.connected()) {
            return false;
        }

        if (nullptr == _central) {
            BLEMIDI_ArduinoBLE::blePeripheralConnectHandler(central);
            _central = &central;
        }
        else {
            if (*_central != central) {
                BLEMIDI_ArduinoBLE::blePeripheralDisconnectHandler(*_central);
                BLEMIDI_ArduinoBLE::blePeripheralConnectHandler(central);
                _central = &central;
            } 
        }

        return true;
    }

	static void blePeripheralConnectHandler(BLEDevice central)
	{
        _central = &central;
		if (_bleMidiTransport->_connectedCallback)
			_bleMidiTransport->_connectedCallback();
	}

	static void blePeripheralDisconnectHandler(BLEDevice central)
	{
		if (_bleMidiTransport->_disconnectedCallback)
			_bleMidiTransport->_disconnectedCallback();

        _central = nullptr;
	}

    static void characteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
        auto buffer = characteristic.value();
        auto length = characteristic.valueLength();

        if (length > 0)
            receive(buffer, length);
    }
};

BLEMIDI_Transport<class BLEMIDI_ArduinoBLE>* BLEMIDI_ArduinoBLE::_bleMidiTransport = nullptr;
BLEDevice* BLEMIDI_ArduinoBLE::_central = nullptr;

bool BLEMIDI_ArduinoBLE::begin(const char* deviceName, BLEMIDI_Transport<class BLEMIDI_ArduinoBLE>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    if (!BLE.begin()) 
        return false;

    BLE.setLocalName(deviceName);

    BLE.setAdvertisedService(midiService);
    midiService.addCharacteristic(midiChar);
    BLE.addService(midiService);

#ifdef BLE_EVENTS
    // assign event handlers for connected, disconnected to peripheral
    BLE.setEventHandler(BLEConnected,    BLEMIDI_ArduinoBLE::blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, BLEMIDI_ArduinoBLE::blePeripheralDisconnectHandler);

    midiChar.setEventHandler(BLEWritten, characteristicWritten);
#endif

  /* Start advertising BLE.  It will start continuously transmitting BLE
     advertising packets and will be visible to remote BLE central devices
     until it receives a new connection */

    // start advertising
    BLE.advertise();
    
    return true;
}

 /*! \brief Create an instance for nRF52 named <DeviceName>
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE> BLE##Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE>, BLEMIDI_NAMESPACE::MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE> &)BLE##Name);

 /*! \brief Create a default instance for nRF52 (Nano 33 BLE) named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
BLEMIDI_CREATE_INSTANCE("BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
