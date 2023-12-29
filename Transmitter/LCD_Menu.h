#ifndef LCD_Menu
#define LCD_Menu
#include "Arduino.h"


// Define the menu options
enum MenuOption {
  HOME,               //Displays channel tempretures and remaining cook time
  OVEN_ID,            //Identifier for data transmission
  TEMPERATURE_SETPOINT,
  COOK_TIME,          
  DATA_INTERVAL_TIME, //Send data once every DATA_INTERVAL_TIMER
  NUM_TC_PER_OVEN,    //Number of Thermocouples to monitor per oven
  NUM_OVENS,          //Number of Ovens being monitored
  BUZZER_MODE,        //Buzzer On/ Off
  TC_CALI_MODE,       //TC Calibration Mode Select (None, TC1, TC2 ...TC8)
  SAVE_PARAM,         //Parameters are saved after the menu is cycled back to Home
  TC_CALI,            //TC Calibration for TCX
};

//Number of menu screens-1 for TC_CALI?  Update when adding a new menu item.
int Num_Menu_Screens = 11;

// Define the current menu option
MenuOption currentMenuOption = HOME;

class Parameter {
public:
  int value;    //Stored value.  A defult Value is assigned if no value is found on SD card Parameters.txt file.  
  int min_val;  //lower Limit for value
  int max_val;  //Upper limit for value

  Parameter(int value, int min_val, int max_val) {
    this->value = value;
    this->min_val = min_val;
    this->max_val = max_val;
  }

  int Value() {
    return value;
  }

  void setValue(int value) {
    this->value = constrain(value, min_val, max_val);
  }

  int getMinVal() {
    return min_val;
  }

  int getMaxVal() {
    return max_val;
  }
};

//System Defults if no parameter.txt file exists
//(Defult value, Min value, max value)
Parameter ovenID(1, 1, 99);                     //The first Oven Number.  Any addational ovens are auto asinged.
Parameter temperatureSetpoint(225, 0, 500);
Parameter cookTime(7, 2, 999);                   //Minutes
Parameter dataIntervalTime(60, 1, 240);          //Seconds
Parameter numTCperOven(1,1,8);                  //Number of RTDs/ Thermocouples per Oven
Parameter numOvens(4,1,8);                      //Number of Ovens connected to each Transmitter
Parameter buzzerMode(1,0,1);                    //Buzzer On/ Off
Parameter tc_cali_mode(0,0,8);                  //Skip Calibration, Calibrate TC1, Cali TC2 .... TC8, Exit to SAVE_PARAM
Parameter tc_offset_1(0,-100,100);              //Thermocouple Offset value.  Display TC_Card.readtemp() + tc_offset.  Record tc_offset + roteryValue
// Parameter cycleNum(0,0,999);                     //counts each time a cycle is started.

int TC_Offsets[9] = {0,0,0,0,0,0,0,0,0};        //9 Elements so tc_cali_mode can be used for indexing.  tc_cali_mode.  First element will allways be 0 

//Degree symbol for use on LCD screen
byte degree[8] = {
  0b01110,
  0b01010,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};


#endif