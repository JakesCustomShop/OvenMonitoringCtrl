/*********
  Jake Lehotsky
  Displays 6 random values to a 20x4 Sequent Microsystem LCD adapter HAT.

*********/

#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 20;
int lcdRows = 4;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x08, lcdColumns, lcdRows);  

void setup(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight(20);
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
  Temp_Disp = String("1: ") + String(x[0]) + String(" F");
  lcd.setCursor(0, 0);
  lcd.print(Temp_Disp);

  Temp_Disp = String("2: ") + String(x[1]) + String(" F");
  lcd.setCursor(0, 1);
  lcd.print(Temp_Disp);

  Temp_Disp = String("3: ") + String(x[2]) + String(" F");
  lcd.setCursor(8, 0);
  lcd.print(Temp_Disp);

  Temp_Disp = String("4: ") + String(x[3]) + String(" F");
  lcd.setCursor(8, 1);
  lcd.print(Temp_Disp);

  delay(2000);
  lcd.clear(); 
  return;
}
