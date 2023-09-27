/*!
*   Blink example for Sequent Microsystems Six-in-one LCD Adapter Kit for Raspberry Pi with a 20X4 LCD 
*   -->https://sequentmicrosystems.com
*   Requirments: Any arduino card with I2C, Six-in-one LCD Adapter Kit from Sequent Microsystems
*
*   Method 1:
*     Connect I2C bus, 5V and GND from Arduino card to Home automation hat, pin map below
*    SIGNAL  CONNECTOR    SIGNAL\n
*     ---------------------------
*          -- 1|O O| 2--  +5V
*  I2C SDA -- 3|O O| 4--  +5V
*  I2C SCL -- 5|O O| 6--  GND
*          -- 7|O O| 8--
*      GND -- 9|O O|10--
*          --11|O O|12--
*          --13|O O|14--  GND
*          --15|O O|16--
*          --17|O O|18--
*          --19|O O|20--  GND
*          --21|O O|22--
*          --23|O O|24--
*      GND --25|O O|26--
*          --27|O O|28--
*          --29|O O|30--  GND
*          --31|O O|32--
*          --33|O O|34--  GND
*          --35|O O|36--
*          --37|O O|38--
*      GND --39|O O|40--
*
*   Method 2:
*   Use sequent Microsystems Uno, Nano, Teensy,Feather, ESP32 Raspberry Pi Replacement Kit with prefered arduino processor
*   -->https://sequentmicrosystems.com/products/raspberry-pi-replacement-card
*
*   Method 3:
*    Use Sequent Microsysstems ESP32 Pi replacement connected directly with the Home Automation card.
*    Set the board type to DOIT ESP32 DEVKIT V1: Tool >> Board >> ESP32 Arduino >> DOIT ESP32 DEVKIT V1
* 
*   Based on Arduino's Blink Example at
*   http://www.arduino.cc/en/Tutorial/LiquidCrystalBlink
*/

#include "SM_LCDAdapter.h"

SM_LCDAdapter lcd = SM_LCDAdapter();

void setup() {

  // set up the LCD's number of columns and rows:

  lcd.begin(20, 4);
  lcd.writeBl(20);

  // Print a message to the LCD.

  lcd.print("hello, world!");
  Serial.begin(115200);
}

typedef struct struct_message {
    int id; // must be unique for each sender board
    int x[8];   //Each element is a seperate chamber in the oven?
    int y[8];
} struct_message;

// Create a struct_message called myData
struct_message myData;


void loop(){
  String Temp_Disp;
  for(int i = 0; i < 8; i++){
    myData.x[i] = random(0,200);
  } 
  DisplayTemps(myData.x);

}




/*
Recieves an array of tempreture readings (up to 4)
and displays them on an LCD screen.  
*/
void DisplayTemps(int x[]) {
  //String Temp_Disp = String(x);
  String Temp_Disp;
  // set cursor to first column, first row
  Temp_Disp = String("1: ") + String(x[0]);
  lcd.setCursor(0, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 0);
  lcd.print("F");

  Temp_Disp = String("2: ") + String(x[1]);
  lcd.setCursor(0, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 1);
  lcd.print("F");
  
  Temp_Disp = String("3: ") + String(x[2]);
  lcd.setCursor(0, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 2);
  lcd.print("F");

  Temp_Disp = String("4: ") + String(x[3]);
  lcd.setCursor(10, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 0);
  lcd.print("F");

  Temp_Disp = String("5: ") + String(x[4]);
  lcd.setCursor(10, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 1);
  lcd.print("F");

  Temp_Disp = String("5: ") + String(x[5]);
  lcd.setCursor(10, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 2);
  lcd.print("F");
  

  delay(2000);
  lcd.clear(); 
  return;
}
