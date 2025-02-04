#pragma once

//#define MIDIBLECLIENTVERBOSE

#ifdef MIDIBLECLIENTVERBOSE
#define DEBUGCLIENT(_text_) Serial.println("DbgBC: " + (String)_text_);
#else
#define DEBUGCLIENT(_text_) ;
#endif

// Headers for ESP32 nimBLE
#include <NimBLEDevice.h>

BEGIN_BLEMIDI_NAMESPACE

using PasskeyRequestCallback = uint32_t (*)(void);

static uint32_t defautlPasskeyRequest()
{
    // FILL WITH YOUR CUSTOM AUTH METHOD CODE or PASSKEY
    // FOR EXAMPLE:
    uint32_t passkey = 123456;

    // Serial.println("Client Passkey Request");

    /** return the passkey to send to the server */
    return passkey;
};

struct DefaultSettingsClient : public BLEMIDI_NAMESPACE::DefaultSettings
{

    /*
    ##### BLE DEVICE NAME  #####
    */

    /**
     * Set name of ble device (not affect to connection with server)
     * max 16 characters
     */
    static constexpr char *name = "BleMidiClient";

    /*
    ###### TX POWER #####
    */

    /**
     * Set power transmision
     *
     * ESP_PWR_LVL_N12                // Corresponding to -12dbm    Minimum
     * ESP_PWR_LVL_N9                 // Corresponding to  -9dbm
     * ESP_PWR_LVL_N6                 // Corresponding to  -6dbm
     * ESP_PWR_LVL_N3                 // Corresponding to  -3dbm
     * ESP_PWR_LVL_N0                 // Corresponding to   0dbm
     * ESP_PWR_LVL_P3                 // Corresponding to  +3dbm
     * ESP_PWR_LVL_P6                 // Corresponding to  +6dbm
     * ESP_PWR_LVL_P9                 // Corresponding to  +9dbm    Maximum
     */
    static const esp_power_level_t clientTXPwr = ESP_PWR_LVL_P9;

    /*
    ###### SECURITY #####
    */

    /** Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY   - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    static const uint8_t clientSecurityCapabilities = BLE_HS_IO_NO_INPUT_OUTPUT;

    /** Set the security method.
     *  bonding
     *  man in the middle protection
     *  pair. secure connections
     *
     *  More info in nimBLE lib
     */
    static const bool clientBond = true;
    static const bool clientMITM = false;
    static const bool clientPair = true;

    /**
     * This callback function defines what will be done when server requieres PassKey.
     * Add your custom code here.
     */
    static constexpr PasskeyRequestCallback userOnPassKeyRequest = defautlPasskeyRequest;

    /*
    ###### BLE COMMUNICATION PARAMS ######
    */

    /** Set connection parameters:
     *  If you only use one connection, put recomended BLE server param communication
     *  (you may scan it ussing "nRF Connect" app or other similar apps).
     *
     *  If you use more than one connection adjust, for example, settings like 15ms interval, 0 latency, 120ms timout.
     *  These settings may be safe for 3 clients to connect reliably, set faster values if you have less
     *  connections.
     *
     *  Min interval (unit: 1.25ms): 12 * 1.25ms = 15 ms,
     *  Max interval (unit: 1.25ms): 12 * 1.25ms = 15,
     *  0 latency (Number of intervals allowed to skip),
     *  TimeOut (unit: 10ms) 51 * 10ms = 510ms. Timeout should be minimum 100ms.
     */
    static const uint16_t commMinInterval = 6;  // 7.5ms
    static const uint16_t commMaxInterval = 35; // 40ms
    static const uint16_t commLatency = 0;      //
    static const uint16_t commTimeOut = 200;    // 2000ms

    /*
    ###### BLE FORCE NEW CONNECTION ######
    */

    /**
     * This parameter force to skip the "soft-reconnection" and force to create a new connection after a disconnect event.
     * "Soft-reconnection" save some time and energy in comparation with performming a new connection, but some BLE devices
     * don't support or don't perform correctly a "soft-reconnection" after a disconnection event.
     * Set to "true" if your device doesn't work propertily after a reconnection.
     *
     */
    static const bool forceNewConnection = false;

    /*
    ###### BLE SUBSCRIPTION: NOTIFICATION & RESPONSE ######
    */

    /**
     * Subscribe in Notification Mode [true] or Indication Mode [false]
     * Don't modify this parameter except is completely necessary.
     */
    static const bool notification = true;
    /**
     * Respond to after a notification message.
     * Don't modify this parameter except is completely necessary.
     */
    static const bool response = true;
};

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
public:
    NimBLEAdvertisedDevice advDevice;
    bool doConnect = false;
    bool scanDone = false;
    bool specificTarget = false;
    bool enableConnection = false;
    std::string nameTarget;

protected:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
        if (!enableConnection) // not begin() or end()
        {
            return;
        }

        DEBUGCLIENT("Advertised Device found: ");
        DEBUGCLIENT(advertisedDevice->toString().c_str());
        if (!advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
        {
            doConnect = false;
            return;
        }

        DEBUGCLIENT("Found MIDI Service");
        if (!(!specificTarget || (advertisedDevice->getName() == nameTarget.c_str() || advertisedDevice->getAddress() == nameTarget)))
        {
            DEBUGCLIENT("Name error");
            return;
        }

        /** Ready to connect now */
        doConnect = true;
        /** Save the device reference in a public variable that the client can use*/
        advDevice = *advertisedDevice;
        /** stop scan before connecting */
        NimBLEDevice::getScan()->stop();

        return;
    };
};

/** Define a funtion to handle the callbacks when scan ends */
void scanEndedCB(NimBLEScanResults results);

/** Define the class that performs Client Midi (nimBLE) */
template <class _Settings>
class BLEMIDI_Client_ESP32
{
private:
    BLEClient *_client = nullptr;
    BLEAdvertising *_advertising = nullptr;
    BLERemoteCharacteristic *_characteristic = nullptr;
    BLERemoteService *pSvc = nullptr;
    bool firstTimeSend = true; //First writeValue get sends like Write with reponse for clean security flags. After first time, all messages are send like WriteNoResponse for increase transmision speed.
    char connectedDeviceName[24];
    
    BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *_bleMidiTransport = nullptr;

    bool specificTarget = false;

    AdvertisedDeviceCallbacks myAdvCB;

protected:
    QueueHandle_t mRxQueue;

public:
    BLEMIDI_Client_ESP32()
    {
    }

    bool begin(const char *, BLEMIDI_Transport<class BLEMIDI_Client_ESP32<_Settings>, _Settings> *);

    bool end()
    {
        myAdvCB.enableConnection = false;
        xQueueReset(mRxQueue);
        _client->disconnect();
        _client = nullptr;

        return true;
    }

    void write(uint8_t *data, uint8_t length)
    {
        if (!myAdvCB.enableConnection)
            return;
        if (_characteristic == NULL)
            return;

        if (firstTimeSend)
        {
            _characteristic->writeValue(data, length, true);
            firstTimeSend = false;
            return;
        }

        if (!_characteristic->writeValue(data, length, !_characteristic->canWriteNoResponse()))
            firstTimeSend = true;

        return;
    }

    bool available(byte *pvBuffer);

    void add(byte value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        xQueueSend(mRxQueue, &value, portMAX_DELAY / 2);
    }

protected:
    void receive(uint8_t *buffer, size_t length)
    {
        // forward the buffer so that it can be parsed
        _bleMidiTransport->receive(buffer, length);
    }

    void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

    void scan();
    bool connect();

public:
    void connected()
    {
        if (_bleMidiTransport->_connectedCallback)
            _bleMidiTransport->_connectedCallback();
        firstTimeSend = true;
        
        if (_bleMidiTransport->_connectedCallbackDeviceName)
        {
            sprintf(connectedDeviceName, "%s", myAdvCB.advDevice.getName().c_str());
            _bleMidiTransport->_connectedCallbackDeviceName(connectedDeviceName);
        }
    }

    void disconnected()
    {
        if (_bleMidiTransport->_disconnectedCallback)
            _bleMidiTransport->_disconnectedCallback();
        firstTimeSend = true;
    }
};

/** Define the class that performs interruption callbacks */
template <class _Settings>
class MyClientCallbacks : public BLEClientCallbacks
{
public:
    MyClientCallbacks(BLEMIDI_Client_ESP32<_Settings> *bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32)
    {
    }

protected:
    BLEMIDI_Client_ESP32<_Settings> *_bluetoothEsp32 = nullptr;

    uint32_t onPassKeyRequest()
    {
        // if (nullptr != _Settings::userOnPassKeyRequest)
        return _Settings::userOnPassKeyRequest();
        // return 0;
    };

    void onConnect(BLEClient *pClient)
    {
        DEBUGCLIENT("##Connected##");
        // pClient->updateConnParams(_Settings::commMinInterval, _Settings::commMaxInterval, _Settings::commLatency, _Settings::commTimeOut);
        vTaskDelay(1);
        if (_bluetoothEsp32)
            _bluetoothEsp32->connected();
    };

    void onDisconnect(BLEClient *pClient)
    {
        DEBUGCLIENT(pClient->getPeerAddress().toString().c_str());
        DEBUGCLIENT(" Disconnected - Starting scan");

        if (_bluetoothEsp32)
        {
            _bluetoothEsp32->disconnected();
        }

        if (_Settings::forceNewConnection)
        {
            // Renew Client
            NimBLEDevice::deleteClient(pClient);
            pClient = nullptr;
        }

        // Try reconnection or search a new one
        NimBLEDevice::getScan()->start(1, scanEndedCB);
    }

    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        if (params->itvl_min < _Settings::commMinInterval)
        { /** 1.25ms units */
            return false;
        }
        else if (params->itvl_max > _Settings::commMaxInterval)
        { /** 1.25ms units */
            return false;
        }
        else if (params->latency > _Settings::commLatency)
        { /** Number of intervals allowed to skip */
            return false;
        }
        else if (params->supervision_timeout > _Settings::commMinInterval)
        { /** 10ms units */
            return false;
        }
        pClient->updateConnParams(params->itvl_min, params->itvl_max, params->latency, params->supervision_timeout);

        return true;
    };
};

/*
##########################################
############# IMPLEMENTATION #############
##########################################
*/

template <class _Settings>
bool BLEMIDI_Client_ESP32<_Settings>::begin(const char *deviceName, BLEMIDI_Transport<class BLEMIDI_Client_ESP32<_Settings>, _Settings> *bleMidiTransport)
{
    _bleMidiTransport = bleMidiTransport;

    std::string strDeviceName(deviceName);
    // Connect to the first midi server found
    if (strDeviceName == "")
    {
        myAdvCB.specificTarget = false;
        myAdvCB.nameTarget = "";
    }
    // Connect to a specific name or address
    else
    {
        myAdvCB.specificTarget = true;
        myAdvCB.nameTarget = strDeviceName;
    }

    static char array[16];
    memcpy(array, _Settings::name, 16);
    strDeviceName = array;
    DEBUGCLIENT(strDeviceName.c_str());
    NimBLEDevice::init(strDeviceName);

    // To communicate between the 2 cores.
    // Core_0 runs here, core_1 runs the BLE stack
    mRxQueue = xQueueCreate(_Settings::MaxBufferSize, sizeof(uint8_t));

    NimBLEDevice::setSecurityIOCap(_Settings::clientSecurityCapabilities); // Attention, it may need a passkey
    NimBLEDevice::setSecurityAuth(_Settings::clientBond, _Settings::clientMITM, _Settings::clientPair);

    /** Optional: set the transmit power, default is 3db */
    NimBLEDevice::setPower(_Settings::clientTXPwr); /** +9db */

    myAdvCB.enableConnection = true;
    scan();

    return true;
}

template <class _Settings>
bool BLEMIDI_Client_ESP32<_Settings>::available(byte *pvBuffer)
{
    if (!myAdvCB.enableConnection)
    {
        return false;
    }

    // Try to connect/reconnect
    if (_client == nullptr || !_client->isConnected())
    {
        if (myAdvCB.doConnect)
        {
            myAdvCB.doConnect = false;
            if (!connect())
            {
                scan();
            }
        }
        else if (myAdvCB.scanDone)
        {
            scan();
        }
    }

    // return 1 byte from the Queue
    return xQueueReceive(mRxQueue, (void *)pvBuffer, 0); // return immediately when the queue is empty
}

/** Notification receiving handler callback */
template <class _Settings>
void BLEMIDI_Client_ESP32<_Settings>::notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (this->_characteristic == pRemoteCharacteristic) // Redundant protection
    {
        receive(pData, length);
    }
}

template <class _Settings>
void BLEMIDI_Client_ESP32<_Settings>::scan()
{
    // Retrieve a Scanner and set the callback you want to use to be informed when a new device is detected.
    // Specify that you want active scanning and start the
    // scan to run for 3 seconds.
    myAdvCB.scanDone = true;
    NimBLEScan *pBLEScan = BLEDevice::getScan();
    if (!pBLEScan->isScanning())
    {
        pBLEScan->setAdvertisedDeviceCallbacks(&myAdvCB);
        pBLEScan->setInterval(600);
        pBLEScan->setWindow(500);
        pBLEScan->setActiveScan(true);

        DEBUGCLIENT("Scanning...");
        pBLEScan->start(1, scanEndedCB);
    }
};

template <class _Settings>
bool BLEMIDI_Client_ESP32<_Settings>::connect()
{
    using namespace std::placeholders; //<- for bind funtion in callback notification

    // Retry to connecto to last one
    if (!_Settings::forceNewConnection)
    {
        /** Check if we have a client we should reuse first
         *  Special case when we already know this device
         *  This saves considerable time and power.
         */

        if (_client)
        {
            if (_client == NimBLEDevice::getClientByPeerAddress(myAdvCB.advDevice.getAddress()))
            {
                if (_client->connect(&myAdvCB.advDevice, false))
                {
                    if (_characteristic->canNotify())
                    {
                        if (_characteristic->subscribe(_Settings::notification, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4), _Settings::response))
                        {
                            // Re-connection SUCCESS
                            return true;
                        }
                    }
                    /** Disconnect if subscribe failed */
                    _client->disconnect();
                }
                /* If any connection problem exits, delete previous client and try again in the next attemp as new client*/
                NimBLEDevice::deleteClient(_client);
                _client = nullptr;
                return false;
            }
            /*If client does not match, delete previous client and create a new one*/
            NimBLEDevice::deleteClient(_client);
            _client = nullptr;
        }
    }

    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
    {
        DEBUGCLIENT("Max clients reached - no more connections available");
        return false;
    }

    // Create and setup a new client
    _client = BLEDevice::createClient();

    _client->setClientCallbacks(new MyClientCallbacks<_Settings>(this), false);

    _client->setConnectionParams(_Settings::commMinInterval, _Settings::commMaxInterval, _Settings::commLatency, _Settings::commTimeOut);

    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    _client->setConnectTimeout(15);

    if (!_client->connect(&myAdvCB.advDevice))
    {
        /** Created a client but failed to connect, don't need to keep it as it has no data */
        NimBLEDevice::deleteClient(_client);
        _client = nullptr;
        DEBUGCLIENT("Failed to connect, deleted client");
        return false;
    }

    if (!_client->isConnected())
    {
        DEBUGCLIENT("Failed to connect");
        _client->disconnect();
        NimBLEDevice::deleteClient(_client);
        _client = nullptr;
        return false;
    }

    DEBUGCLIENT("Connected to: " + myAdvCB.advDevice.getName().c_str() + " / " + _client->getPeerAddress().toString().c_str());

    DEBUGCLIENT("RSSI: ");
    DEBUGCLIENT(_client->getRssi());

    /** Now we can read/write/subscribe the charateristics of the services we are interested in */
    pSvc = _client->getService(SERVICE_UUID);
    if (pSvc) /** make sure it's not null */
    {
        _characteristic = pSvc->getCharacteristic(CHARACTERISTIC_UUID);

        if (_characteristic) /** make sure it's not null */
        {
            if (_characteristic->canNotify())
            {
                if (_characteristic->subscribe(_Settings::notification, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4), _Settings::response))
                {
                    // Connection SUCCESS
                    return true;
                }
            }
        }
    }

    // If anything fails, disconnect and delete client
    _client->disconnect();
    NimBLEDevice::deleteClient(_client);
    _client = nullptr;
    return false;
};

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results)
{
    // DEBUGCLIENT("Scan Ended");
}

END_BLEMIDI_NAMESPACE

/*! \brief Create a custom instance for ESP32 named <DeviceName>, and advertise it like "Prefix + <DeviceName> + Subfix"
    It will try to connect to a specific server with equal name or addr than <DeviceName>. If <DeviceName> is "", it will connect to first midi server
 */
#define BLEMIDI_CREATE_CUSTOM_INSTANCE(DeviceName, Name, _Settings)                                                            \
    BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32<_Settings>, _Settings> BLE##Name(DeviceName); \
    MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32<_Settings>, _Settings>, BLEMIDI_NAMESPACE::MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32<_Settings>, _Settings> &)BLE##Name);

/*! \brief Create an instance for ESP32 named <DeviceName>, and advertise it like "Prefix + <DeviceName> + Subfix"
    It will try to connect to a specific server with equal name or addr than <DeviceName>. If <DeviceName> is "", it will connect to first midi server
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
    BLEMIDI_CREATE_CUSTOM_INSTANCE(DeviceName, Name, BLEMIDI_NAMESPACE::DefaultSettingsClient)

/*! \brief Create a default instance for ESP32 named BLEMIDI-CLIENT.
    It will try to connect to first midi ble server found.
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
    BLEMIDI_CREATE_INSTANCE("", MIDI)
