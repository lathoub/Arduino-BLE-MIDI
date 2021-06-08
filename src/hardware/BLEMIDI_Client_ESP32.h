#pragma once

// Headers for ESP32 nimBLE
#include <NimBLEDevice.h>

static NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static bool scanDone = false;
/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
public:
    int test = 0;

protected:
    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
        test++;
        Serial.println(test);
        Serial.print("Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
        {
            Serial.println("Found Our Service");
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            advDevice = advertisedDevice;
            /** Ready to connect now */
            doConnect = true;
        }
        else
        {
            doConnect = false;
        }
    };
};

void scanEndedCB(NimBLEScanResults results);

BEGIN_BLEMIDI_NAMESPACE

class BLEMIDI_Client_ESP32
{
private:
    BLEClient *_client = nullptr;
    BLEAdvertising *_advertising = nullptr;
    BLERemoteCharacteristic *_characteristic = nullptr;
    BLERemoteService *pSvc = nullptr;
    //NimBLERemoteDescriptor *pDsc = nullptr;

    BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *_bleMidiTransport = nullptr;

    friend class AdvertisedDeviceCallbacks;
    friend class MyClientCallbacks;
    friend class MIDI_NAMESPACE::MidiInterface<BLEMIDI_Transport<BLEMIDI_Client_ESP32>, MySettings>;//

    AdvertisedDeviceCallbacks myAdvCB;

protected:
    QueueHandle_t mRxQueue;

public:
    BLEMIDI_Client_ESP32()
    {
    }

    bool begin(const char *, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *);

    void write(uint8_t *data, uint8_t length)
    {
        _characteristic->writeValue(data, length, true);
        myAdvCB.test++;
    }

    bool available(byte *pvBuffer)
    {
        if (_client == nullptr || !_client->isConnected())
        {
            //Serial.println("No Conectado");
            if (doConnect)
            {
                doConnect = false;
                Serial.println("intentar conexion");
                if (connect())
                {
                    Serial.println("reconnected");                   
                }
                else
                {
                    Serial.println("Rescan");
                    scanDone=false;
                    NimBLEDevice::getScan()->start(3, scanEndedCB);
                }
            }
            else if (scanDone)
            {   
                scanDone=false;
                Serial.println("Rescan 2");
                NimBLEDevice::getScan()->start(3, scanEndedCB);
            }
        }

        // return 1 byte from the Queue
        return xQueueReceive(mRxQueue, (void *)pvBuffer, 0); // return immediately when the queue is empty
    }

    void
    add(byte value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        xQueueSend(mRxQueue, &value, portMAX_DELAY);
    }

    void receive(uint8_t *buffer, size_t length)
    {
        // forward the buffer so it can be parsed
        _bleMidiTransport->receive(buffer, length);
    }

    void connectCallbacks(MIDI_NAMESPACE::MidiInterface<BLEMIDI_Transport<BLEMIDI_Client_ESP32>, MySettings> *MIDIcallback);

    void connected()
    {
        Serial.println("!!");
        if (_bleMidiTransport->_connectedCallback)
        {
            Serial.println("@");
            _bleMidiTransport->_connectedCallback();
        }
    }

    void disconnected()
    {
        if (_bleMidiTransport->_disconnectedCallback)
            _bleMidiTransport->_disconnectedCallback();
    }

    void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

    void scan();
    bool connect();
};

class MyClientCallbacks : public BLEClientCallbacks
{
public:
    MyClientCallbacks(BLEMIDI_Client_ESP32 *bluetoothEsp32)
        : _bluetoothEsp32(bluetoothEsp32)
    {
    }

protected:
    BLEMIDI_Client_ESP32 *_bluetoothEsp32 = nullptr;

    void onConnect(BLEClient *pClient)
    {
        Serial.println("##Connected##");
        /** After connection we should change the parameters if we don't need fast response times.
         *  These settings are 150ms interval, 0 latency, 450ms timout.
         *  Timeout should be a multiple of the interval, minimum is 100ms.
         *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
         */
        //pClient->updateConnParams
        pClient->updateConnParams(6, 32, 0, 40);
        if (_bluetoothEsp32)
        {
            Serial.println("??");
            _bluetoothEsp32->connected();
        }
    };

    void onDisconnect(BLEClient *pClient)
    {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");

        if (_bluetoothEsp32)
        {
            _bluetoothEsp32->disconnected();
        }

        NimBLEDevice::getScan()->start(3, scanEndedCB);
    }
    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        if (params->itvl_min < 24)
        { /** 1.25ms units */
            return false;
        }
        else if (params->itvl_max > 40)
        { /** 1.25ms units */
            return false;
        }
        else if (params->latency > 2)
        { /** Number of intervals allowed to skip */
            return false;
        }
        else if (params->supervision_timeout > 100)
        { /** 10ms units */
            return false;
        }

        return true;
    };
};

/** Notification / Indication receiving handler callback */
void BLEMIDI_Client_ESP32::notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    // std::string str = (isNotify == true) ? "Notification" : "Indication";
    // str += " from ";
    // /** NimBLEAddress and NimBLEUUID have std::string operators */
    // str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    // str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    // str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
    // str += ", Value = " + std::string((char *)pData, length);
    // Serial.println(str.c_str());

    receive(pData, length);
}

void BLEMIDI_Client_ESP32::scan()
{
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    NimBLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(&myAdvCB);
    pBLEScan->setInterval(1500);
    pBLEScan->setWindow(500);
    pBLEScan->setActiveScan(true);
    //doScan = true;
    Serial.println("Scanning...");
    pBLEScan->start(10, scanEndedCB);
};

bool BLEMIDI_Client_ESP32::connect()
{
    Serial.println("TryConnection...");
    using namespace std::placeholders;
    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getClientListSize())
    {
        /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        _client = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (_client)
        {
            if (!_client->connect(advDevice, false))
            {
                Serial.println("Reconnect failed");
                return false;
            }
            Serial.println("Reconnected client");
            _client->setConnectionParams(12, 12, 0, 51);
            if (_characteristic->canNotify())
                {
                    Serial.println("CAN NOTIFY");
                    if (!_characteristic->subscribe(true, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                    {
                        Serial.println("error");
                        /** Disconnect if subscribe failed */
                        _client->disconnect();
                        return false;
                    }
                }
        }
        /** We don't already have a client that knows this device,
         *  we will check for a client that is disconnected that we can use.
         */
        else
        {
            _client = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if (!_client)
    {
        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
        {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }
        _client = BLEDevice::createClient();

        _client->setClientCallbacks(new MyClientCallbacks(this), false);
        /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
         */
        _client->setConnectionParams(12, 12, 0, 51);
        /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
        _client->setConnectTimeout(5);
        Serial.println("#Connection");
        if (!_client->connect(advDevice))
        {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            
            NimBLEDevice::deleteClient(_client);
            _client=nullptr;
            Serial.println("Failed to connect, deleted client");
            return false;
        }
        vTaskDelay(100);

        if (!_client->isConnected())
        {
            if (!_client->connect(advDevice))
            {
                Serial.println("Failed to connect");
                return false;
            }
        }
        else
        {

            Serial.print("Connected to: ");
            Serial.println(_client->getPeerAddress().toString().c_str());
            Serial.print("RSSI: ");
            Serial.println(_client->getRssi());
        }
        /** Now we can read/write/subscribe the charateristics of the services we are interested in */

        pSvc = _client->getService(SERVICE_UUID);
        if (pSvc)
        { /** make sure it's not null */
            _characteristic = pSvc->getCharacteristic(CHARACTERISTIC_UUID);

            if (_characteristic)
            { /** make sure it's not null */
                // if (_characteristic->canRead())
                // {
                //     Serial.print(_characteristic->getUUID().toString().c_str());
                //     Serial.print(" Value: ");
                //     Serial.println(_characteristic->readValue().c_str());
                // }

                /** registerForNotify() has been deprecated and replaced with subscribe() / unsubscribe().
             *  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
             *  Unsubscribe parameter defaults are: response=false.
             */
                if (_characteristic->canNotify())
                {
                    Serial.println("CAN NOTIFY");
                    if (!_characteristic->subscribe(true, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                    {
                        Serial.println("error");
                        /** Disconnect if subscribe failed */
                        _client->disconnect();
                        return false;
                    }
                }
                // else if (_characteristic->canIndicate())
                // {
                //     /** Send false as first argument to subscribe to indications instead of notifications */
                //     //if(!pChr->registerForNotify(notifyCB, false)) {
                //     if (!_characteristic->subscribe(false, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                //     {
                //         /** Disconnect if subscribe failed */
                //         _client->disconnect();
                //         return false;
                //     }
                // }
            }
        }
        else
        {
            Serial.println("MIDI service not found.");
            return false;
        }
    }
    return true;
};

bool BLEMIDI_Client_ESP32::begin(const char *deviceName, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *bleMidiTransport)
{

    _bleMidiTransport = bleMidiTransport;

    Serial.println("CreateBLE");
    std::string stringDeviceName(deviceName);
    NimBLEDevice::init(stringDeviceName);
    Serial.println("CREATED");

    // To communicate between the 2 cores.
    // Core_0 runs here, core_1 runs the BLE stack
    mRxQueue = xQueueCreate(64, sizeof(uint8_t)); // TODO Settings::MaxBufferSize
    Serial.println("QUEUE");
    /** Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY    - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

    /** 2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, secure connections.
     *
     *  These are the default values, only shown here for demonstration.
     */
    //NimBLEDevice::setSecurityAuth(false, false, true);
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | */ BLE_SM_PAIR_AUTHREQ_SC);
    Serial.println("Security");
    /** Optional: set the transmit power, default is 3db */
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    Serial.println("Power");
    /** Optional: set any devices you don't want to get advertisments from */
    // NimBLEDevice::addIgnored(NimBLEAddress ("aa:bb:cc:dd:ee:ff"));

    scan();
    vTaskDelay(1000);

    //connect();

    // while (!doConnect)
    // {

    //     if (!pBLEScan->isScanning())
    //     {
    //         Serial.println("RESCAN");
    //         pBLEScan->start(10, scanEndedCB);
    //     }
    //     vTaskDelay(100);
    // }
    // Serial.println("LOOP OUT");

    return true;
}

END_BLEMIDI_NAMESPACE

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results)
{
    Serial.println("Scan Ended");
    if (!doConnect)
    {
        scanDone = true;
    }
    else
    {
        scanDone = false;
        NimBLEDevice::getScan()->start(3, scanEndedCB);
    }
}

/*! \brief Create an instance for ESP32 named <DeviceName>
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name)                                                        \
    BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32> BLE##Name(DeviceName); \
    MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32>, BLEMIDI_NAMESPACE::MySettings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_Client_ESP32> &)BLE##Name);

/*! \brief Create a default instance for ESP32 named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
    BLEMIDI_CREATE_INSTANCE("Esp32-NimBLE-MIDI", MIDI)
