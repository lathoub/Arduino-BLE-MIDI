#pragma once

#include <ArduinoBLE.h>

BLEService midiService(SERVICE_UUID);
BLEStringCharacteristic midiChar(CHARACTERISTIC_UUID,  // standard 16-bit characteristic UUID
    BLERead | BLEWrite | BLENotify | BLEWriteWithoutResponse, 16); // remote clients will be able to get notifications if this characteristic changes

#define BLE_POLLING

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI
{
private:   
    static BLEMIDITransport<class BLEMIDI>* _bleMidiTransport;
    static BLEDevice* _central;

//    byte mRxBuffer[Settings::MaxBufferSize];

public:
	BLEMIDI()
    {
    }
    
	bool begin(const char*, BLEMIDITransport<class BLEMIDI>*);
    
    void write(uint8_t* buffer, size_t length)
    {
        // TODO: test length
        ((BLECharacteristic)midiChar).writeValue(buffer, length);
    }
    
    size_t available(uint8_t* buffer, const size_t index,  const size_t max)
    {
 #ifdef BLE_POLLING
        poll();
    
        if (midiChar.written()) {
          //  auto buffer = midiChar.value();
            auto length = midiChar.valueLength();

            if (length > 0) {
                // TODO: test length
                memcpy(buffer + index, midiChar.value().c_str(), length);
                return length;
            }
        }
#endif
#ifdef BLE_EVENTS
/      BLE.poll();
        return 0;
#endif
    }

    void read()
    {
    }

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
	static void receive(const uint8_t* buffer, size_t length)
	{
        Serial.print("receive: ");
        for (int i = 0; i < length; i++) {
            Serial.print(" 0x");
            Serial.print(buffer[i], HEX);
        }
        Serial.println("");
	}

	bool poll()
	{
        BLEDevice central = BLE.central();
        if (!central) {
            if (_central) {
                BLEMIDI::blePeripheralDisconnectHandler(*_central);
                _central = nullptr;
            }
            return false;
        }

        if (!central.connected()) {
            return false;
        }

        if (nullptr == _central) {
            BLEMIDI::blePeripheralConnectHandler(central);
            _central = &central;
        }
        else {
            if (*_central != central) {
                BLEMIDI::blePeripheralDisconnectHandler(*_central);
                BLEMIDI::blePeripheralConnectHandler(central);
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

BLEMIDITransport<class BLEMIDI>* BLEMIDI::_bleMidiTransport = nullptr;
BLEDevice* BLEMIDI::_central = nullptr;

bool BLEMIDI::begin(const char* deviceName, BLEMIDITransport<class BLEMIDI>* bleMidiTransport)
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
    BLE.setEventHandler(BLEConnected,    BLEMIDI::blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, BLEMIDI::blePeripheralDisconnectHandler);

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
BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI> BLE##Name(DeviceName); \
MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI>, MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDITransport<BLEMIDI_NAMESPACE::BLEMIDI> &)BLE##Name);

 /*! \brief Create a default instance for nRF52 named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
BLEMIDI_CREATE_INSTANCE("BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
