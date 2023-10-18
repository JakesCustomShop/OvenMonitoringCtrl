#ifndef LCD_Menu
#define LCD_Menu
#include "Arduino.h"

// Define the menu options
enum MenuOption {
  HOME,       //Displays channel tempretures and remaining cook time
  OVEN_ID,
  TEMPERATURE_SETPOINT,
  COOK_TIME,
  DATA_INTERVAL_TIME
};

// Define the current menu option
MenuOption currentMenuOption = HOME;

class Parameter {
public:
  unsigned int value;
  unsigned int min_val;
  unsigned int max_val;

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

Parameter ovenID(1, 1, 8);
Parameter temperatureSetpoint(225, 0, 500);
Parameter cookTime(7, 1, 60);            //Seconds
Parameter dataIntervalTime(3000, 1000, 100000); //Miliseconds

#endif

