#include <BLEMIDI_Transport.h>

static uint32_t customPasskeyRequest()
{
    // FILL WITH YOUR CUSTOM AUTH METHOD CODE or PASSKEY
    // FOR EXAMPLE:
    uint32_t passkey = 123456;

    // Serial.println("Client Passkey Request");

    /** return the passkey to send to the server */
    return passkey;
};

 struct CustomBufferSizeSettings : public BLEMIDI_NAMESPACE::DefaultSettings {
  //See all options and them explanation in the library.
  
  /*
  ##### BLE DEVICE NAME  #####
  */
  //static constexpr char *name = "BleMidiClient";
  /*
  ###### TX POWER #####
  */
  //static const esp_power_level_t clientTXPwr = ESP_PWR_LVL_P9;
  /*
  ###### SECURITY #####
  */
  //static const uint8_t clientSecurityCapabilities = BLE_HS_IO_NO_INPUT_OUTPUT;
  //static const bool clientBond = true;
  //static const bool clientMITM = false;
  //static const bool clientPair = true;
  //static constexpr PasskeyRequestCallback userOnPassKeyRequest = customPasskeyRequest;
  /*
  ###### BLE COMMUNICATION PARAMS ######
  */
  //static const uint16_t commMinInterval = 6;  // 7.5ms
  //static const uint16_t commMaxInterval = 35; // 40ms
  //static const uint16_t commLatency = 0;      //
  //static const uint16_t commTimeOut = 200;    // 2000ms
  /*
  ###### BLE FORCE NEW CONNECTION ######
  */
  //static const bool forceNewConnection = false;
  /*
  ###### BLE SUBSCRIPTION: NOTIFICATION & RESPONSE ######
  */
  //static const bool notification = true;
  //static const bool response = true;
  /*
  ###### AND THE OTHER SETTINGS OF MIDI LIBRARY ######
  */
  static const size_t MaxBufferSize = 16;
  
};

#include <hardware/BLEMIDI_ESP32_NimBLE.h>
//#include <hardware/BLEMIDI_ESP32.h>
//#include <hardware/BLEMIDI_ArduinoBLE.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

BLEMIDI_CREATE_CUSTOM_INSTANCE("Esp32-NimBLE-MIDI", MIDI, CustomBufferSizeSettings);

unsigned long t0 = millis();
bool isConnected = false;


// -----------------------------------------------------------------------------
// When BLE connected, LED will turn on (indication that connection was successful)
// When receiving a NoteOn, LED will go out, on NoteOff, light comes back on.
// This is an easy and conveniant way to show that the connection is alive and working.
// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("booting");

  MIDI.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  BLEMIDI.setHandleConnected([]() {
    isConnected = true;
    digitalWrite(LED_BUILTIN, HIGH);
  });

  BLEMIDI.setHandleDisconnected([]() {
    isConnected = false;
    digitalWrite(LED_BUILTIN, LOW);
  });

  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    digitalWrite(LED_BUILTIN, LOW);
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    digitalWrite(LED_BUILTIN, HIGH);
  });
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  MIDI.read();

  if (isConnected && (millis() - t0) > 1000)
  {
    t0 = millis();

    MIDI.sendNoteOn (60, 100, 1); // note 60, velocity 100 on channel 1
  }
}
