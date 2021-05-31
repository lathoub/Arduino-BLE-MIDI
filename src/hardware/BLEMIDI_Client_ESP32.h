#pragma once

// Headers for ESP32 nimBLE
#include <NimBLEDevice.h>

static NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static bool scanDone = false;
/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
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
    BLECharacteristic *_characteristic = nullptr;

    BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *_bleMidiTransport = nullptr;

    friend class AdvertisedDeviceCallbacks;

protected:
    QueueHandle_t mRxQueue;

public:
    BLEMIDI_Client_ESP32()
    {
    }

    bool begin(const char *, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *);

    void write(uint8_t *data, uint8_t length)
    {
        _characteristic->setValue(data, length);
        //_characteristic->notify();
    }

    bool available(byte *pvBuffer)
    {
        // return 1 byte from the Queue
        return xQueueReceive(mRxQueue, (void *)pvBuffer, 0); // return immediately when the queue is empty
    }

    void add(byte value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        xQueueSend(mRxQueue, &value, portMAX_DELAY);
    }

    void receive(uint8_t *buffer, size_t length)
    {
        // forward the buffer so it can be parsed
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

    void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
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
        Serial.println("Connected");
        /** After connection we should change the parameters if we don't need fast response times.
         *  These settings are 150ms interval, 0 latency, 450ms timout.
         *  Timeout should be a multiple of the interval, minimum is 100ms.
         *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
         */
        pClient->updateConnParams(6, 32, 0, 40);
        if (_bluetoothEsp32)
            _bluetoothEsp32->connected();
    };

    void onDisconnect(BLEClient *pClient)
    {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
        
        
        
        if (_bluetoothEsp32){
            _bluetoothEsp32->disconnected();
        }

        NimBLEDevice::getScan()->start(3,scanEndedCB);
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

bool BLEMIDI_Client_ESP32::begin(const char *deviceName, BLEMIDI_Transport<class BLEMIDI_Client_ESP32> *bleMidiTransport)
{

    using namespace std::placeholders;
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

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.

    NimBLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setInterval(150);
    pBLEScan->setWindow(50);
    pBLEScan->setActiveScan(true);
    //doScan = true;
    Serial.println("Scan");
    pBLEScan->start(10, scanEndedCB);
    Serial.println("Scan2");

    while (!doConnect)
    {

        if (!pBLEScan->isScanning())
        {
            Serial.println("RESCAN");
            pBLEScan->start(10, scanEndedCB);
        }
        vTaskDelay(100);
    }
    Serial.println("LOOP OUT");

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
    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;
    //NimBLERemoteDescriptor *pDsc = nullptr;

    pSvc = _client->getService(SERVICE_UUID);
    if (pSvc)
    { /** make sure it's not null */
        pChr = pSvc->getCharacteristic(CHARACTERISTIC_UUID);

        if (pChr)
        { /** make sure it's not null */
            if (pChr->canRead())
            {
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" Value: ");
                Serial.println(pChr->readValue().c_str());
            }

            /** registerForNotify() has been deprecated and replaced with subscribe() / unsubscribe().
             *  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
             *  Unsubscribe parameter defaults are: response=false.
             */
            if (pChr->canNotify())
            {
                Serial.println("CAN NOTIFY");
                if (!pChr->subscribe(true, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                {
                    Serial.println("error");
                    /** Disconnect if subscribe failed */
                    _client->disconnect();
                    return false;
                }
            }
            else if (pChr->canIndicate())
            {
                /** Send false as first argument to subscribe to indications instead of notifications */
                //if(!pChr->registerForNotify(notifyCB, false)) {
                if (!pChr->subscribe(false, std::bind(&BLEMIDI_Client_ESP32::notifyCB, this, _1, _2, _3, _4)))
                {
                    /** Disconnect if subscribe failed */
                    _client->disconnect();
                    return false;
                }
            }
        }
    }
    else
    {
        Serial.println("MIDI service not found.");
        return false;
    }

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
         NimBLEDevice::getScan()->start(3,scanEndedCB);
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
