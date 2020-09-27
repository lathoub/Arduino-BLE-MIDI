#pragma once

// Headers for nRF52 BLE
//#include "BLECharacteristic.h"
//#include "BLEService.h"

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI
{
private:
        
    BLEMIDITransport<class BLEMIDI>* _bleMidiTransport = nullptr;

public:
	BLEMIDI()
    {
    }
    
	bool begin(const char*, BLEMIDITransport<class BLEMIDI>*);
    
    void write(uint8_t* buffer, size_t length)
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
	void receive(uint8_t* buffer, size_t length)
	{
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

bool BLEMIDI::begin(const char* deviceName, BLEMIDITransport<class BLEMIDI>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    // Config the peripheral connection with maximum bandwidth 
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
//    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  

//    Bluefruit.begin();
//    Bluefruit.setName(deviceName);
//    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

    // Setup the on board blue LED to be enabled on CONNECT
//    Bluefruit.autoConnLed(true);

    // Configure and Start Device Information Service
//    bledis.setManufacturer("Adafruit Industries");
//    bledis.setModel("Bluefruit Feather52");
  //  bledis.begin();

    // Start advertising
    // Set General Discoverable Mode flag
//    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);

    // Advertise TX Power
//    Bluefruit.Advertising.addTxPower();

    // Advertise BLE MIDI Service
//    Bluefruit.Advertising.addService(blemidi);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
//    Bluefruit.ScanResponse.addName();

    /* Start Advertising
    * - Enable auto advertising if disconnected
    * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
    * - Timeout for fast mode is 30 seconds
    * - Start(timeout) with timeout = 0 will advertise forever (until connected)
    *
    * For recommended advertising interval
    * https://developer.apple.com/library/content/qa/qa1931/_index.html   
    */
//    Bluefruit.Advertising.restartOnDisconnect(true);
//    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
//    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
//    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
    
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
