#include "BleMidi.h"

BLEMIDI_CREATE_INSTANCE(BLEMIDI);

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
  // Serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.print(F("Getting IP address..."));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{

}
