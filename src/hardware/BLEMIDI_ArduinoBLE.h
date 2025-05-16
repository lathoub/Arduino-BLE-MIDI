#pragma once

#include <ArduinoBLE.h>

#define BLE_POLLING

BEGIN_BLEMIDI_NAMESPACE

// Dependanced class settings
struct DefaultSettings : public _DefaultSettings
{
    //TODO Create parametric configurations
};

BLEService midiService(SERVICE_UUID);

BLEStringCharacteristic midiChar(CHARACTERISTIC_UUID,  // standard 16-bit characteristic UUID
    BLERead | BLEWrite | BLENotify | BLEWriteWithoutResponse, 16); // remote clients will be able to get notifications if this characteristic changes

template<typename T, int rawSize>
class Fifo {
public:
    const size_t size; // speculative feature, in case it's needed

    Fifo() : size(rawSize)
    {
        flush();
    }

    T dequeue()
    {
        numberOfElements--;
        nextOut %= size;
        return raw[nextOut++];
    };

    bool enqueue(T element)
    {
        if (count() >= rawSize)
            return false;

        numberOfElements++;
        nextIn %= size;
        raw[nextIn] = element;
        nextIn++; // advance to next index

        return true;
    };

    T peek() const
    {
        return raw[nextOut % size];
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

template <class _Settings>
class BLEMIDI_ArduinoBLE
{
private:
    BLEMIDI_Transport<class BLEMIDI_ArduinoBLE<_Settings>, _Settings> *_bleMidiTransport;
    BLEDevice *_central;

    BLEService _midiService;
    BLECharacteristic _midiChar;

    Fifo<byte, _Settings::MaxBufferSize> mRxBuffer;

    template <class> friend class MyServerCallbacks;

public:
    BLEMIDI_ArduinoBLE() : _midiService(SERVICE_UUID),
                           _midiChar(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify | BLEWriteWithoutResponse, _Settings::MaxBufferSize)
    {
    }

    bool begin(const char *, BLEMIDI_Transport<class BLEMIDI_ArduinoBLE<_Settings>, _Settings> *);

    void end()
    {
    }

    void write(uint8_t *buffer, size_t length)
    {
        if (length > 0)
            ((BLECharacteristic)_midiChar).writeValue(buffer, length);
    }

    bool available(byte *pvBuffer)
    {
        if (mRxBuffer.count() > 0)
        {
            *pvBuffer = mRxBuffer.dequeue();
            return true;
        }

        if (_midiChar.written())
        {
            auto length = _midiChar.valueLength();
            if (length > 0)
            {
                auto buffer = _midiChar.value();
                _bleMidiTransport->receive((byte *)buffer, length);
            }
        }
        return false;
    }

    void add(byte value)
    {
        // called from BLE-MIDI, to add it to a buffer here
        mRxBuffer.enqueue(value);
    }

protected:
    void receive(const unsigned char *buffer, size_t length)
    {
        if (length > 0)
            _bleMidiTransport->receive((uint8_t *)buffer, length);
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

        end();
    }
};

template <class _Settings>
class MyServerCallbacks : public BLEDeviceCallbacks
{
public:
    MyServerCallbacks(BLEMIDI_ArduinoBLE<_Settings> *bluetooth)
        : _bluetooth(bluetooth)
    {
    }

protected:
    BLEMIDI_ArduinoBLE<_Settings> *_bluetooth = nullptr;

    void onConnect(BLEDevice device)
    {
        if (_bluetooth)
            _bluetooth->connected();
    };

    void onDisconnect(BLEDevice device)
    {
        if (_bluetooth)
            _bluetooth->disconnected();
    }
};

template <class _Settings>
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
public:
    MyCharacteristicCallbacks(BLEMIDI_ArduinoBLE<_Settings> *bluetooth)
        : _bluetooth(bluetooth)
    {
    }

protected:
    BLEMIDI_ArduinoBLE<_Settings> *_bluetooth = nullptr;

    void onWrite(BLECharacteristic characteristic)
    {
    //    std::string rxValue = characteristic->getValue();
   //     if (rxValue.length() > 0)
     //   {
       //     _bluetooth->receive((uint8_t *)(rxValue.c_str()), rxValue.length());
        //}
    }

    void onRead(BLECharacteristic characteristic)
    {
    }
};

template <class _Settings>
bool BLEMIDI_ArduinoBLE<_Settings>::begin(const char *deviceName, BLEMIDI_Transport<class BLEMIDI_ArduinoBLE<_Settings>, _Settings> *bleMidiTransport)
{
    _bleMidiTransport = bleMidiTransport;

    // initialize the Bluetooth® Low Energy hardware
    if (!BLE.begin())
        return false;

    BLE.setLocalName(deviceName);

    BLE.setAdvertisedService(_midiService);
    _midiService.addCharacteristic(_midiChar);
    BLE.addService(_midiService);

    BLE.setCallbacks(new MyServerCallbacks<_Settings>(this));

    _midiChar.setCallbacks(new MyCharacteristicCallbacks<_Settings>(this));

    // set the initial value for the characeristic:
    // (when not set, the device will disconnect after 0.5 seconds)
    _midiChar.writeValue((uint8_t)0); // TODO: might not be needed, check!!

    /* Start advertising BLE.  It will start continuously transmitting BLE
       advertising packets and will be visible to remote BLE central devices
       until it receives a new connection */
    BLE.advertise();

    return true;
}

/*! \brief Create an instance for ArduinoBLE <DeviceName>
 */
#define BLEMIDI_CREATE_CUSTOM_INSTANCE(DeviceName, Name, _Settings)                                                          \
    BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE<_Settings>, _Settings> BLE##Name(DeviceName); \
    MIDI_NAMESPACE::MidiInterface<BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE<_Settings>, _Settings>, _Settings> Name((BLEMIDI_NAMESPACE::BLEMIDI_Transport<BLEMIDI_NAMESPACE::BLEMIDI_ArduinoBLE<_Settings>, _Settings> &)BLE##Name);

/*! \brief Create an instance for ArduinoBLE <DeviceName>
 */
#define BLEMIDI_CREATE_INSTANCE(DeviceName, Name) \
    BLEMIDI_CREATE_CUSTOM_INSTANCE(DeviceName, Name, BLEMIDI_NAMESPACE::DefaultSettings);

/*! \brief Create a default instance for ArduinoBLE named BLE-MIDI
 */
#define BLEMIDI_CREATE_DEFAULT_INSTANCE() \
    BLEMIDI_CREATE_INSTANCE("BLE-MIDI", MIDI)

END_BLEMIDI_NAMESPACE
