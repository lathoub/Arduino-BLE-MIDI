#pragma once

/*
#############################################
########## USER DEFINES BEGINNING ###########
####### Modify only these parameters ########
#############################################
*/

/*
##### BLE DEVICE NAME  #####
*/

/**
 * Set always the same name independently of name server
 */
//#define BLEMIDI_CLIENT_FIXED_NAME       "BleMidiClient"

#ifndef BLEMIDI_CLIENT_FIXED_NAME //Not modify
/**
 * When client tries to connect to specific server, BLE name is composed as follows:
 * BLEMIDI_CLIENT_NAME_PREFIX + <NameServer/addrServer> + BLEMIDI_CLIENT_NAME_SUBFIX
 * 
 * example: 
 * BLEMIDI_CLIENT_NAME_PREFIX   "Client-"
 * <NameServer/addrServer>      "AX-Edge"
 * BLEMIDI_CLIENT_NAME_SUBFIX   "-Midi1"
 * 
 * Result: "Client-AX-Edge-Midi1"
 */
#define BLEMIDI_CLIENT_NAME_PREFIX "C-"
#define BLEMIDI_CLIENT_NAME_SUBFIX ""

/**
 * When client tries to connect to the first midi server found:
 */
#define BLEMIDI_CLIENT_DEFAULT_NAME "BLEMIDI-CLIENT"
#endif //Not modify

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

#define BLEMIDI_TX_PWR ESP_PWR_LVL_P9

/*
###### SECURITY #####
*/

/** Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY   - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
#define BLEMIDI_CLIENT_SECURITY_CAP BLE_HS_IO_NO_INPUT_OUTPUT

/** Set the security method.
     *  bonding
     *  man in the middle protection
     *  pair. secure connections
     * 
     *  More info in nimBLE lib
     * 
     *  Uncomment what you need
     *  These are the default values.
     *  You can select some simultaneously.
     */
#define BLEMIDI_CLIENT_BOND
//#define BLEMIDI_CLIENT_MITM
#define BLEMIDI_CLIENT_PAIR

/**
 * This callback function defines what will be done when server requieres PassKey.
 * Add your custom code here.
 */
static uint32_t userOnPassKeyRequest()
{
    //FILL WITH YOUR CUSTOM AUTH METHOD CODE or PASSKEY
    //FOR EXAMPLE:
    uint32_t passkey = 123456;

    //Serial.println("Client Passkey Request");

    /** return the passkey to send to the server */
    return passkey;
};

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
#define BLEMIDI_CLIENT_COMM_MIN_INTERVAL 6  // 7.5ms
#define BLEMIDI_CLIENT_COMM_MAX_INTERVAL 35 // 40ms
#define BLEMIDI_CLIENT_COMM_LATENCY 0       //
#define BLEMIDI_CLIENT_COMM_TIMEOUT 200     //2000ms

/*
###### BLE FORCE NEW CONNECTION ######
*/

/** 
 * This parameter force to skip the "soft-reconnection" and force to create a new connection after a disconnect event.
 * "Soft-reconnection" save some time and energy in comparation with performming a new connection, but some BLE devices 
 * don't support or don't perform correctly a "soft-reconnection" after a disconnection event.
 * Uncomment this define if your device doesn't work propertily after a reconnection.
 * 
*/
//#define BLEMIDI_FORCE_NEW_CONNECTION

/**
 * 
*/

/*
#############################################
############ USER DEFINES END ###############
#############################################
*/

// Headers for ESP32 nimBLE
#include <NimBLEDevice.h>

BEGIN_BLEMIDI_NAMESPACE

#ifdef BLEMIDI_CLIENT_BOND
#define BLEMIDI_CLIENT_BOND_DUMMY BLE_SM_PAIR_AUTHREQ_BOND
#else
#define BLEMIDI_CLIENT_BOND_DUMMY 0x00
#endif

#ifdef BLEMIDI_CLIENT_MITM
#define BLEMIDI_CLIENT_MITM_DUMMY BLE_SM_PAIR_AUTHREQ_MITM
#else
#define BLEMIDI_CLIENT_MITM_DUMMY 0x00
#endif

#ifdef BLEMIDI_CLIENT_PAIR
#define BLEMIDI_CLIENT_PAIR_DUMMY BLE_SM_PAIR_AUTHREQ_SC
#else
#define BLEMIDI_CLIENT_PAIR_DUMMY 0x00
#endif

/** Set the security method.
     *  bonding
     *  man in the middle protection
     *  pair. secure connections
     * 
     *  More info in nimBLE lib
     */
#define BLEMIDI_CLIENT_SECURITY_AUTH (BLEMIDI_CLIENT_BOND_DUMMY | BLEMIDI_CLIENT_MITM_DUMMY | BLEMIDI_CLIENT_PAIR_DUMMY)

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
        if (enableConnection) //not begin() or end()
        {
            Serial.print("Advertised Device found: ");
            Serial.println(advertisedDevice->toString().c_str());
            if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
            {
                Serial.println("Found MIDI Service");
                if (!specificTarget || (advertisedDevice->getName() == nameTarget.c_str() || advertisedDevice->getAddress() == nameTarget))
                {
                    /** Ready to connect now */
                    doConnect = true;
                    /** Save the device reference in a public variable that the client can use*/
                    advDevice = *advertisedDevice;
                    /** stop scan before connecting */
                    NimBLEDevice::getScan()->stop();
                }
                else
                {
                    Serial.println("Name error");
                }
            }
            else
            {
                doConnect = false;
            }
        }
    };
};

/** Define a funtion to handle the callbacks when scan ends */
void scanEndedCB(NimBLEScanResults results);

/** Define the class that performs Client Midi (nimBLE) */
class BLEMIDI_Client_ESP32
{
private:
    BLEClient *_client = nullptr;
    BLEAdvertising *_advertising = nullptr;
    BLERemoteCharacteristic *_characteristic = nullptr;
    BLERemoteService *pSvc = nullptr;
    bool firstTimeSend = true; //First writeValue get sends like Write with reponse for clean security flags. After first time, all messages are send like WriteNoResponse for increase transmision speed.

    BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *_bleMidiTransport = nullptr;

    bool specificTarget = false;

    friend class AdvertisedDeviceCallbacks;
    friend class MyClientCallbacks;
    friend class MIDI_NAMESPACE::MidiInterface<BLEMIDI_Transport<BLEMIDI_Client_ESP32>, MySettings>; //

    AdvertisedDeviceCallbacks myAdvCB;

protected:
    QueueHandle_t mRxQueue;

public:
    BLEMIDI_Client_ESP32()
    {
    }

    bool begin(const char *, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *);

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

    void connectCallbacks(MIDI_NAMESPACE::MidiInterface<BLEMIDI_Transport<BLEMIDI_Client_ESP32>, MySettings> *MIDIcallback);

    void connected()
    {
        if (_bleMidiTransport->_connectedCallback)
        {
            _bleMidiTransport->_connectedCallback();
        }
        firstTimeSend = true;
    }

    void disconnected()
    {
        if (_bleMidiTransport->_disconnectedCallback)
        {
            _bleMidiTransport->_disconnectedCallback();
        }
        firstTimeSend = true;
    }

    void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

    void scan();
    bool connect();
};

/** Define the class that performs interruption callbacks */
class MyClientCallbacks : public BLEClientCallbacks
{
public:
    MyClientCallbacks(BLEMIDI_Client_ESP32 *bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32)
    {
    }

protected:
    BLEMIDI_Client_ESP32 *_bluetoothEsp32 = nullptr;

    uint32_t onPassKeyRequest()
    {
        return userOnPassKeyRequest();
    };

    void onConnect(BLEClient *pClient)
    {
        //Serial.println("##Connected##");
        //pClient->updateConnParams(BLEMIDI_CLIENT_COMM_MIN_INTERVAL, BLEMIDI_CLIENT_COMM_MAX_INTERVAL, BLEMIDI_CLIENT_COMM_LATENCY, BLEMIDI_CLIENT_COMM_TIMEOUT);
        vTaskDelay(1);
        if (_bluetoothEsp32)
        {
            _bluetoothEsp32->connected();
        }
    };

    void onDisconnect(BLEClient *pClient)
    {
        //Serial.print(pClient->getPeerAddress().toString().c_str());
        //Serial.println(" Disconnected - Starting scan");

        if (_bluetoothEsp32)
        {
            _bluetoothEsp32->disconnected();
#ifdef BLEMIDI_FORCE_NEW_CONNECTION
            // Try reconnection or search a new one
            _bluetoothEsp32->scan();
#endif // BLEMIDI_FORCE_NEW_CONNECTION
        }

#ifdef BLEMIDI_FORCE_NEW_CONNECTION
        // Renew Client
        NimBLEDevice::deleteClient(pClient);
        NimBLEDevice::createClient();
        pClient = nullptr;
#endif // BLEMIDI_FORCE_NEW_CONNECTION

        //Try reconnection or search a new one
        NimBLEDevice::getScan()->start(1, scanEndedCB);
    }

    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        if (params->itvl_min < BLEMIDI_CLIENT_COMM_MIN_INTERVAL)
        { /** 1.25ms units */
            return false;
        }
        else if (params->itvl_max > BLEMIDI_CLIENT_COMM_MAX_INTERVAL)
        { /** 1.25ms units */
            return false;
        }
        else if (params->latency > BLEMIDI_CLIENT_COMM_LATENCY)
        { /** Number of intervals allowed to skip */
            return false;
        }
        else if (params->supervision_timeout > BLEMIDI_CLIENT_COMM_TIMEOUT)
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

bool BLEMIDI_Client_ESP32::begin(const char *deviceName, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *bleMidiTransport)
{
    _bleMidiTransport = bleMidiTransport;

    std::string strDeviceName(deviceName);
    if (strDeviceName == "") // Connect to the first midi server found
    {
        myAdvCB.specificTarget = false;
        myAdvCB.nameTarget = "";

#ifdef BLEMIDI_CLIENT_FIXED_NAME
        strDeviceName = BLEMIDI_CLIENT_FIXED_NAME;
#else
        strDeviceName = BLEMIDI_CLIENT_DEFAULT_NAME;
#endif
    }
    else // Connect to a specific name or address
    {
        myAdvCB.specificTarget = true;
        myAdvCB.nameTarget = strDeviceName;

#ifdef BLEMIDI_CLIENT_FIXED_NAME
        strDeviceName = BLEMIDI_CLIENT_FIXED_NAME;
#else
        strDeviceName = BLEMIDI_CLIENT_NAME_PREFIX + strDeviceName + BLEMIDI_CLIENT_NAME_SUBFIX;
#endif
    }
    Serial.println(strDeviceName.c_str());

    NimBLEDevice::init(strDeviceName);

    // To communicate between the 2 cores.
    // Core_0 runs here, core_1 runs the BLE stack
    mRxQueue = xQueueCreate(256, sizeof(uint8_t)); // TODO Settings::MaxBufferSize

    NimBLEDevice::setSecurityIOCap(BLEMIDI_CLIENT_SECURITY_CAP); // Attention, it may need a passkey
    NimBLEDevice::setSecurityAuth(BLEMIDI_CLIENT_SECURITY_AUTH);

    /** Optional: set the transmit power, default is 3db */
    NimBLEDevice::setPower(BLEMIDI_TX_PWR); /** +9db */

    myAdvCB.enableConnection = true;
    scan();

    return true;
}

bool BLEMIDI_Client_ESP32::available(byte *pvBuffer)
{
    if (myAdvCB.enableConnection)
    {
        if (_client == nullptr || !_client->isConnected()) //Try to connect/reconnect
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
    else
    {
        return false;
    }
}

/** Notification receiving handler callback */
void BLEMIDI_Client_ESP32::notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (this->_characteristic == pRemoteCharacteristic) //Redundant protection
    {
        receive(pData, length);
    }
}

void BLEMIDI_Client_ESP32::scan()
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

        Serial.println("Scanning...");
        pBLEScan->start(1, scanEndedCB);
    }
};

bool BLEMIDI_Client_ESP32::connect()
{
    using namespace std::placeholders; //<- for bind funtion in callback notification

#ifndef BLEMIDI_FORCE_NEW_CONNECTION
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
                    if (_characteristic->subscribe(true, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                    {
                        //Re-connection SUCCESS
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
#endif //BLEMIDI_FORCE_NEW_CONNECTION

    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
    {
        Serial.println("Max clients reached - no more connections available");
        return false;
    }

    // Create and setup a new client
    _client = BLEDevice::createClient();

    _client->setClientCallbacks(new MyClientCallbacks(this), false);

    _client->setConnectionParams(BLEMIDI_CLIENT_COMM_MIN_INTERVAL, BLEMIDI_CLIENT_COMM_MAX_INTERVAL, BLEMIDI_CLIENT_COMM_LATENCY, BLEMIDI_CLIENT_COMM_TIMEOUT);

    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    _client->setConnectTimeout(15);

    if (!_client->connect(&myAdvCB.advDevice))
    {
        /** Created a client but failed to connect, don't need to keep it as it has no data */
        NimBLEDevice::deleteClient(_client);
        _client = nullptr;
        //Serial.println("Failed to connect, deleted client");
        return false;
    }

    if (!_client->isConnected())
    {
        //Serial.println("Failed to connect");
        _client->disconnect();
        NimBLEDevice::deleteClient(_client);
        _client = nullptr;
        return false;
    }

    Serial.print("Connected to: ");
    Serial.print(myAdvCB.advDevice.getName().c_str());
    Serial.print(" / ");
    Serial.println(_client->getPeerAddress().toString().c_str());

    /*
    Serial.print("RSSI: ");
    Serial.println(_client->getRssi());
    */

    /** Now we can read/write/subscribe the charateristics of the services we are interested in */
    pSvc = _client->getService(SERVICE_UUID);
    if (pSvc) /** make sure it's not null */
    {
        _characteristic = pSvc->getCharacteristic(CHARACTERISTIC_UUID);

        if (_characteristic) /** make sure it's not null */
        {
            if (_characteristic->canNotify())
            {
                if (_characteristic->subscribe(true, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                {
                    //Connection SUCCESS
                    return true;
                }
            }
        }
    }

    //If anything fails, disconnect and delete client
    _client->disconnect();
    NimBLEDevice::deleteClient(_client);
    _client = nullptr;
    return false;
};

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results)
{
    // Serial.println("Scan Ended");
}

END_BLEMIDI_NAMESPACE

/*! \brief Create an instance for ESP32 named <DeviceName>, and adviertise it like "Prefix + <DeviceName> + Subfix"
    It will try to connect to a specific server with equal name or addr than <DeviceName>. If <DeviceName> is "", it will connect to first midi server
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name)                                                        \
    BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32> BLE##Name(DeviceName); \
    MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32>, BLEMIDI_NAMESPACE::MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32> &)BLE##Name);

/*! \brief Create a default instance for ESP32 named BLEMIDI-CLIENT. 
    It will try to connect to first midi ble server found.
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
    BLEMIDI_CREATE_INSTANCE("", MIDI)
