#ifndef LCD_Menu
#define LCD_Menu

#include "Arduino.h"

// Define the menu options
enum MenuOption {
  HOME,
  OVEN_ID,
  TEMPERATURE_SETPOINT,
  COOK_TIME,
  DATA_INTERVAL_TIME
};

// Define the current menu option
MenuOption currentMenuOption = HOME;

class Parameter {
public:
  int value;
  int min_val;
  int max_val;

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
Parameter temperatureSetpoint(350, 100, 500);
Parameter cookTime(60, 1, 6000);
Parameter dataIntervalTime(3, 1, 1000);

#endif
