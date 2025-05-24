#pragma once

// I N   D E V E L O P M E N T

#include <bluefruit.h>

BEGIN_BLEMIDI_NAMESPACE

// Dependanced class settings
struct BLEDefaultSettings : public CommonBLEDefaultSettings
{
    //TODO Create parametric configurations
};

template <class _Settings>
class BLEMIDI_nRF52
{
private:
    BLEDis bledis;
  //  BLEMidi blemidi;

    BLEMIDI_NAMESPACE::BLEMIDI_Transport<class BLEMIDI_nRF52<_Settings>, _Settings>* _bleMidiTransport;

 //   template <class> friend class MyServerCallbacks;
 //   template <class> friend class MyCharacteristicCallbacks;

public:
	BLEMIDI_nRF52()
    {
    }
    
	bool begin(const char*, BLEMIDI_NAMESPACE::BLEMIDI_Transport<class BLEMIDI_nRF52<_Settings>, _Settings>*);
    
    void end() 
    {
        
    }

    void write(uint8_t* buffer, size_t length)
    {
    }

    bool available(byte* pvBuffer)
    {
        return false;
    }

    void add(byte value)
    {
    }


protected:    
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

void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");

  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

template <class _Settings>
bool BLEMIDI_nRF52<_Settings>::begin(const char* deviceName, BLEMIDI_NAMESPACE::BLEMIDI_Transport<class BLEMIDI_nRF52<_Settings>, _Settings>* bleMidiTransport)
{
	_bleMidiTransport = bleMidiTransport;

    // Config the peripheral connection with maximum bandwidth 
    // more SRAM required by SoftDevice
    // Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  

    Bluefruit.begin();
    Bluefruit.setName(deviceName);
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

    // Setup the on board blue LED to be enabled on CONNECT
    Bluefruit.autoConnLed(true);

    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);  

    // Configure and Start Device Information Service
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel("Bluefruit Feather52");
    bledis.begin();

    // Start advertising ----------------------------

    // Set General Discoverable Mode flag
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);

    // Advertise TX Power
    Bluefruit.Advertising.addTxPower();

    // Advertise BLE MIDI Service
    Bluefruit.Advertising.addService(blemidi);

  //  blemidi.write((uint8_t)0);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
    * - Enable auto advertising if disconnected
    * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
    * - Timeout for fast mode is 30 seconds
    * - Start(timeout) with timeout = 0 will advertise forever (until connected)
    *
    * For recommended advertising interval
    * https://developer.apple.com/library/content/qa/qa1931/_index.html   
    */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
    
    return true;
}

 /*! \brief Create an instance for nRF52 named <DeviceName>
 */
#define BLEMIDI_CREATE_CUSTOM_INSTANCE(DeviceName, Name, _Settings) \
    BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_nRF52<_Settings>, _Settings> BLE##Name(DeviceName); \
    MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_nRF52<_Settings>, _Settings>, _Settings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_nRF52<_Settings>, _Settings> &)BLE##Name);

 /*! \brief Create an instance for nRF52 named <DeviceName>
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
    BLEMIDI_CREATE_CUSTOM_INSTANCE (DeviceName, Name, BLEMIDI_NAMESPACE::BLEDefaultSettings)

 /*! \brief Create a default instance for nRF52 named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
    BLEMIDI_CREATE_INSTANCE("nRF85BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
