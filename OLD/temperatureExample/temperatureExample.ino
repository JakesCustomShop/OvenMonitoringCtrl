/*!
    Temperature reading usage example for Sequent Microsystems RTD Data Acquisition HAT
    -->https://github.com/SequentMicrosystems/Sequent-RTD-Library
    Requirments: Any arduino card with I2C, RTD Data Acquisition HAT from Sequent Microsystems
    The example is more usefull if you connect one RTD sensors to all channels.

    Note: Must use 3-Wire RTD or jumper on RTD DAQ. inputs
*/
#include "SM_RTD.h"

SM_RTD card(0);// RTD Data Acquisition HAT with stack level 0 (no jumpers installed)

void setup() {
  Serial.begin(115200);
  delay(2000);

  if (card.begin() )
  {
    Serial.print("RTD HAT detected\n");
  }
  else
  {
    Serial.print("RTD HAT NOT detected!\n");
  }
}


void loop() {
  for (int i = 1; i < 9; i++) // 8 inputs
  {
    Serial.printf("%.2f | ", card.readTemp(i));
  }
  Serial.printf("\n");
  delay (2000);
}
