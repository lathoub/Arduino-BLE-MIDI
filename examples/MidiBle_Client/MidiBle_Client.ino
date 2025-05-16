/**
 * --------------------------------------------------------
 * This example shows how to use client MidiBLE
 * Client BLEMIDI works im a similar way Server (Common) BLEMIDI, but with some exception.
 * 
 * The most importart exception is read() method. This function works as usual, but 
 * now it manages machine-states BLE connection too. The
 * read() function must be called several times continuously in order to scan BLE device
 * and connect with the server. In this example, read() is called in a "multitask function of 
 * FreeRTOS", but it can be called in loop() function as usual.
 * 
 * Some BLEMIDI_CREATE_INSTANCE() are added in MidiBLE-Client to be able to choose a specific server to connect
 * or to connect to the first server which has the MIDI characteristic. You can choose the server by typing in the name field
 * the name of the server or the BLE address of the server. If you want to connect 
 * to the first MIDI server BLE found by the device, you just have to set the name field empty ("").
 * 
 * FOR ADVANCED USERS: Other advanced BLE configurations can be changed with a struct that heritate 
 * from BLEMIDI_NAMESPACE::DefaultSettingsClient. These configurations are related to
 * security (password, pairing and securityCallback()), communication params, the device name 
 * and other stuffs. Modify those settings at your own risk.
 * 
 * 
 * 
 * @auth RobertoHE 
 * --------------------------------------------------------
 */

#include <Arduino.h>
#include <BLEMIDI_Transport.h>

#include <hardware/BLEMIDI_Client_ESP32.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// For customPasskeyRequest callback format type
using PasskeyRequestCallback = uint32_t (*)(void);

// This function is called when the server request a passkey (if you need it)
// If yo need a passkey, you must set the security capabilities and set PasskeyRequestCallback

/*static uint32_t customPasskeyRequest()
{
    // FILL WITH YOUR CUSTOM AUTH METHOD CODE or PASSKEY
    // FOR EXAMPLE:
    uint32_t passkey = 123456;

    // Serial.println("Client Passkey Request");

    // return the passkey to send to the server
    return passkey;
};
*/

//See DefaultSettingsClient in hardware/BLEMIDI_Client_ESP32.h for more configurable settings
// If you do not redefine a parameter, it will use the default value for these parameter
struct CustomSettings : public BLEMIDI_NAMESPACE::DefaultSettings
{
  //See all options and them explanation in the library.
  
  /*
  ##### BLE DEVICE NAME  #####
  */
  //static constexpr char *name = (char*)"BleMidiClient";
  /*
  ###### TX POWER #####
  */
  //static const int8_t clientTXPwr = 9; // in dBm
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
  static const bool forceNewConnection = false;
  /*
  ###### BLE SUBSCRIPTION: NOTIFICATION & RESPONSE ######
  */
  //static const bool notification = true;
  //static const bool response = true;
  

  //BLE-MIDI settings (see BLEMIDI_Settings.h and BLE_class for more details)
  static const size_t MaxBufferSize = 16;

  // MIDI settings (see MIDI_Settings.h)
  static const int Use1ByteParsing = true;
  static const bool HandleNullVelocityNoteOnAsNoteOff = true; 
};

BLEMIDI_CREATE_CUSTOM_INSTANCE("Esp32-BLE-MIDI", MIDI, CustomSettings); // Connect to a server named "Esp32-BLE-MIDI" and use CustomSettings as settings of client

//BLEMIDI_CREATE_INSTANCE("",MIDI)                  //Connect to the first server found, using default settings
//BLEMIDI_CREATE_INSTANCE("f2:c1:d9:36:e7:6b",MIDI) //Connect to a specific BLE address server, using default settings
//BLEMIDI_CREATE_INSTANCE("MyBLEserver",MIDI)       //Connect to a specific name server, using default settings

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 //modify for match with yout board
#endif

void ReadCB(void *parameter);       //Continuos Read function (See FreeRTOS multitasks)

unsigned long t0 = millis();
bool isConnected = false;

/**
 * -----------------------------------------------------------------------------
 * When BLE is connected, LED will turn on (indicating that connection was successful)
 * When receiving a NoteOn, LED will go out, on NoteOff, light comes back on.
 * This is an easy and conveniant way to show that the connection is alive and working.
 * -----------------------------------------------------------------------------
*/
void setup()
{
  Serial.begin(115200);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  BLEMIDI.setHandleConnected([]()
                             {
                               Serial.println("---------CONNECTED---------");
                               isConnected = true;
                               digitalWrite(LED_BUILTIN, HIGH);
                             });

  BLEMIDI.setHandleDisconnected([]()
                                {
                                  Serial.println("---------NOT CONNECTED---------");
                                  isConnected = false;
                                  digitalWrite(LED_BUILTIN, LOW);
                                });

  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity)
                       {
                         Serial.print("NoteON: CH: ");
                         Serial.print(channel);
                         Serial.print(" | ");
                         Serial.print(note);
                         Serial.print(", ");
                         Serial.println(velocity);
                         digitalWrite(LED_BUILTIN, LOW);
                       });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity)
                        {
                        digitalWrite(LED_BUILTIN, HIGH);
                         });

  xTaskCreatePinnedToCore(ReadCB,           //See FreeRTOS for more multitask info  
                          "MIDI-READ",
                          3000,
                          NULL,
                          1,
                          NULL,
                          1); //Core0 or Core1

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
    //MIDI.read();  // This function is called in the other task

  if (isConnected && (millis() - t0) > 1000)
  {
    t0 = millis();

    MIDI.sendNoteOn(60, 100, 1); // note 60, velocity 100 on channel 1
    vTaskDelay(250/portTICK_PERIOD_MS);
    MIDI.sendNoteOff(60, 0, 1);
  }
}

/**
 * This function is called by xTaskCreatePinnedToCore() to perform a multitask execution.
 * In this task, read() is called every millisecond (approx.).
 * read() function performs connection, reconnection and scan-BLE functions.
 * Call read() method repeatedly to perform a successfull connection with the server 
 * in case connection is lost.
*/
void ReadCB(void *parameter)
{
//  Serial.print("READ Task is started on core: ");
//  Serial.println(xPortGetCoreID());
  for (;;)
  {
    MIDI.read(); 
    vTaskDelay(1 / portTICK_PERIOD_MS); //Feed the watchdog of FreeRTOS.
    //Serial.println(uxTaskGetStackHighWaterMark(NULL)); //Only for debug. You can see the watermark of the free resources assigned by the xTaskCreatePinnedToCore() function.
  }
  vTaskDelay(1);
}
