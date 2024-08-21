/*!
    Jake Lehotsky
    Jake's Custom Shop, LLC
    August, 2023

    Built for Sequent Microsystems ESP-Pi 
    Reads Tempature data from up to 8 TCs on the Sequent Microsystems TC HAT.
    Transmits Tempature data to a reciever via ESP_now wireless protocall.
    Displays 6  values to a 20x4 Sequent Microsystem LCD adapter HAT.

    Notes: 
     - For use with Reciever3.ino
     - Will work with a plain ol' ESP32 for testing purposes.  
     - Adding a menu option requres update to updateMenuOption() 


  Version 1.1 Changes [Needs Tested]
   - Added Boot screen with Version name (Requires manual updateing)
   - Deisplay8Temps is no longer used
   - I think that setting the Cook Time to 0 will allow for continuous dataloging.  
     - Min value changed from 2 Minutes to 0 Minutes.
   - Fixed detection of 8-relay card.  
   - added "Use8RelayCard" variable to determine where stacklights are.
   - Changed TC calibration values and display to intergers only
   - SM LCD hat & hard-ware buttons both start/ stop timers.
   - setting cook time of 0 turns off the timer and should keep collecting data.


  Version 1.2 Changes
   - Oven tempretures are internally multipied by 10 so that we can have tenths digit precision.  This inclued transmissited values.
   - TC calibration values are saved as 10x their actual value.  
     
*/


/*
TODO:
 - Do somthing about conveyor oven having 2 puchbuttons
  - Implement a responce back to the .py for automatic error checking of param setting.
 - Make sure parameter saving still works.

  Necessary Changes for implemention
   - Set TC_CAL to use tenths of a degree.  This will be a decent project as all temperature data is in integers
   - Change maximum cook time to >= 1000 hours
   - Add SD card data logging.
      - Add file naming for SD card data saving?
   - Implement tempreture monitoring incase of low-temp.
   - How to: Display tempretures if NUM_TC_PER_OVEN = 2 and NUM_OVENS = 2? 
   - How to determine if we are using the 4IO or 8Relay card for Stacklights?
  
*/


#include <Arduino.h>
#include "Timer.h"    //Custom Header and C++ file for the Timing System
#include <esp_now.h>
#include <WiFi.h>

#include "SM_TC.h"
#include "SM_LCDAdapter.h"
#include "LCD_Menu.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "SM_ESP32Pi.h"
#include "SM_4REL4IN.h"
#include "SM_8relay.h"


SM_LCDAdapter lcd = SM_LCDAdapter();

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0x94,0xB9,0x7E,0xF9,0x3A,0x18};

SM_4REL4IN four_IO_card0(0);                //Four Relays four HV Inputs HAT with stack level 0 (no Jmper Installed)
SM_TC TC_Card(1);                           //TC Data Acquisition HAT with stack level 1 (1 jumper installed)
SM_8relay eight_relay_card2 = SM_8relay() ; //Eight Relays HAT with stack level 2
SM_8relay eight_relay_card3 = SM_8relay();  //Eight Relays HAT with stack level 3 


int debug = 0;  //Turns off addational serial print fuctions.
// int debug = 1;    //Basic Debuging.
bool debug2 = 0; //Startup Delays, 
bool TC_Check;    //Check to see if the Sequent TC hat exists.  If FALSE, then enable random number TC value generation
int status[8] = {0,0,0,0,0,0,0,0};      //Status byte for each oven.
int prev_status[8] = {1,1,1,1,1,1,1,1}; //Previous status for determinging when to update a stack light. Anything except STARTUP?
int cycleNum[8]= {0,0,0,0,0,0,0,0};     //Cycle counter for each oven.  
bool Use8RelayCard = false;             //Systems with >1 stacklight use eight_realy_cardX.  Systems with 1 stack light use four_IO_card0.
int TC_Offsets[9] = {0,0,0,0,0,0,0,0,0};        //9 Elements so tc_cali_mode can be used for indexing.  tc_cali_mode.  First element will allways be 0.  10x actual value for 0.1 precision

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int OvenID = 1; // must be unique for each sender board. 
    int Count = 0;    //Counts the number of transmissiogitns.
    int Temps[8] =  {0,0,0,0,0,0,0,0};   //Each element is a seperate chamber in the oven. 
    int Status = 0;

    //int BatchID;     //6-Digit identification number for each batch. 
} struct_message;


struct_message myData;

enum SystemStatus {
  STARTUP,              //Oven temp below SetPoint.  
  OVEN_READY,
  CYCLE_ACTIVE,
  TIME_COMPLETE,
  ACKNOWLEDGED,
  TEMPERATURE_DATA_SAVED,
  ERROR                 
};
// Define the current menu option
//SystemStatus myData.Status = STARTUP;



//Stacklight relay output values.
enum StackLight {
  OFF,    //0
  BUZZ,   //1
  GRN,    //2
  ORG,    //3
  RED     //4
};


esp_now_peer_info_t peerInfo; // Create peer interface

//===Initilize timers===//
//A timer interval of 0 disables the timer
Timer timer[6];
//Create a timer for sending data packets every XX miliseconds z
Timer dataIntervalTimer;
//millis() based timer for blinking the stack lights. 
Timer StackLightOnTimer[4];    
Timer StackLightOffTimer[4];



// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 


//===SD Card Stuff===//
void writeFile(fs::FS &fs, const String path, String message){
  if (debug){Serial.printf("Writing file: "); Serial.println(path);}

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const String path, String message){
  if (debug){Serial.print("Appending to file: "); Serial.println(path);}

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void readParameterFile(fs::FS &fs, const char * path){
  if (debug)Serial.printf("Reading file: ");  Serial.println(path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  // Read the parameters from the file
  String line;
  while (file.available()) {
    line = file.readStringUntil('\n');
    line.trim();

    // Split the line into the parameter name and value
    int colonIndex = line.indexOf(':');
    String parameterName = line.substring(0, colonIndex);
    String parameterValue = line.substring(colonIndex + 1);

    // Convert the parameter value to an integer
    int intValue = parameterValue.toInt();

    // Assign the parameter value to the corresponding Parameter object
    if (parameterName == "ovenID") {
      ovenID.setValue(intValue);
    } else if (parameterName == "temperatureSetpoint") {
      temperatureSetpoint.setValue(intValue);
    } else if (parameterName == "cookTime") {
      cookTime.setValue(intValue);
    } else if (parameterName == "dataIntervalTime") {
      dataIntervalTime.setValue(intValue);
    } else if (parameterName == "numTCperOven") {
      numTCperOven.setValue(intValue);
    } else if (parameterName == "numOvens") {
      numOvens.setValue(intValue);
    } else if (parameterName == "buzzerMode") {
      buzzerMode.setValue(intValue);
    }else if (parameterName == "tcOffset1") {
      TC_Offsets[1] = intValue;
    }else if (parameterName == "tcOffset2") {
      TC_Offsets[2] = intValue;
    }else if (parameterName == "tcOffset3") {
      TC_Offsets[3] = intValue;
    }else if (parameterName == "tcOffset4") {
      TC_Offsets[4] = intValue;
    }else if (parameterName == "tcOffset5") {
      TC_Offsets[5] = intValue;
    }else if (parameterName == "tcOffset6") {
      TC_Offsets[6] = intValue;
    }else if (parameterName == "tcOffset7") {
      TC_Offsets[7] = intValue;
    }else if (parameterName == "tcOffset8") {
      TC_Offsets[8] = intValue;
    }else if (parameterName == "cycleNum")  { 
      // cycleNum.SetValue(intValue);
    }
  }

  // Print the parameter values to the serial monitor
  if (debug){
    Serial.println("Parameter values:");
    Serial.print("ovenID: ");
    Serial.println(ovenID.Value());
    Serial.print("temperatureSetpoint: ");
    Serial.println(temperatureSetpoint.Value());
    Serial.print("cookTime: ");
    Serial.println(cookTime.Value());
    Serial.print("dataIntervalTime: ");
    Serial.println(dataIntervalTime.Value());
    Serial.print("numTCperOven: ");
    Serial.println(numTCperOven.Value());
    Serial.print("numOvens: ");
    Serial.println(numOvens.Value());
    Serial.print("buzzerMode: ");
    Serial.println(buzzerMode.Value());
    Serial.print("tcOffsets: ");
    for (int i=1;i<9;i++){
      Serial.println(TC_Offsets[i]);
      // Serial.print(' ');
    }
    Serial.println();
  }


  file.close();
}

/*
@brief Formats Configuration values into a string, then writes them to the SD Card
@param none
*/
void saveParametersToSD(){
  String parameters = "";
  parameters += "ovenID: " + String(ovenID.Value()) + "\n";
  myData.OvenID = ovenID.Value();     //Please fix this
  parameters += "temperatureSetpoint: " + String(temperatureSetpoint.Value()) + "\n";
  parameters += "cookTime: " + String(cookTime.Value()) + "\n";
  parameters += "dataIntervalTime: " + String(dataIntervalTime.Value()) + "\n";
  parameters += "numTCperOven: " + String(numTCperOven.Value()) + "\n";
  parameters += "numOvens: " + String(numOvens.Value()) + "\n";
  parameters += "buzzerMode: " + String(buzzerMode.Value()) + "\n";
  parameters += "tcOffset1: " + String(TC_Offsets[1]) + "\n";       //saved as 10x value to preserve decimal precision
  parameters += "tcOffset2: " + String(TC_Offsets[2]) + "\n";
  parameters += "tcOffset3: " + String(TC_Offsets[3]) + "\n";
  parameters += "tcOffset4: " + String(TC_Offsets[4]) + "\n";
  parameters += "tcOffset5: " + String(TC_Offsets[5]) + "\n";
  parameters += "tcOffset6: " + String(TC_Offsets[6]) + "\n";
  parameters += "tcOffset7: " + String(TC_Offsets[7]) + "\n";
  parameters += "tcOffset8: " + String(TC_Offsets[8]) + "\n";
  // parameters += "cycleNum: " + String(cycleNum.Value()) + "\n";
  Serial.print(parameters); 
  writeFile(SD, "/parameters.txt", parameters);
  readParameterFile(SD, "/parameters.txt");
}


/*
  @brief Creates a new .csv data file for oven j.  
  Gives the new data file .csv headers
  @param j - oven to create a new data file for
*/
void newDatafile(int j){
  String dataHeader = "Oven #, Sample #, Channel 1,Channel 2,Channel 3,Channel 4,Channel 5,Channel 6,Channel 7,Channel 8,Status \n";
  String file_name = "/"+String(myData.OvenID) + "-sampleData-" + String(cycleNum[j]) +".txt";
  if (!SD.open(file_name))                  //If the file does not exist
    writeFile(SD, file_name, dataHeader);   //Create a new file
  else {
    Serial.println("File already Exists!");
    cycleNum[j]+=1;  //Increment to the next cycleNum.
  }
    //Appened data here?
}


String SerialSendData(struct_message myData) { 
  String Data_String = "";
  Data_String += String(myData.OvenID);
  Data_String += ", ";
  Data_String += String(myData.Count);
  Data_String += ", ";
  for (int i = 0; i < 8; i++) {
    Data_String += String(myData.Temps[i]);   //Data transmitted 10x the measured value to preserve tenth digit precision.
    Data_String += ", ";
  }
  Data_String += String(myData.Status);

  //Send data over serial port to PC GUI
  Serial.print("<");
  Serial.print(Data_String);
  Serial.println(">");

  //End the current row of data for the SD card data log.
  Data_String += "\n";    
  return Data_String;
}



/*
  @brief Lookup function to determine which relay to turn on
  @param oven Oven number 0-3. Should be expanded to 8 ovens for future
  @param color enum Stacklight BUZZ(1), GRN(2), ORG(3), or RED(4).
  @param state LOW or HIGH  
*/
void manageRelays(byte oven, int color, bool state) {
  if (Use8RelayCard){
    //{Relay Card, PinNumber}
    int card_pin_matrix[4][4][2] =          
        //BUZZ, GRN, ORG,  RED
      { {{0,1},{0,2},{0,3},{0,4},},  //Card 2, Oven 0
        {{0,5},{0,6},{0,7},{0,8},},  //Card 2, Oven 1
        {{1,1},{1,2},{1,3},{1,4},},  //Card 3, Oven 2
        {{1,5},{1,6},{1,7},{1,8},},}; //Card 3, Oven 3

    
    int card = card_pin_matrix[oven][color-1][0];   //StackLight Color is inedxed at 1
    int pin = card_pin_matrix[oven][color-1][1];
    Serial.printf("Oven: %d, Color: %d", oven, color);
    Serial.printf("Card: %d, Pin: %d, State: %d", card, pin, state);
    Serial.println();

    switch (card) 
    {
    case 0:   //Relay Card 0
      eight_relay_card2.writeChannel(pin, state);    
      break;
    case 1:   //Relay Card 1
      eight_relay_card3.writeChannel(pin, state);
      break;
    default:
      break;
    }
    delay(10);
  }//if (Use8RelayCard)
  else{   //Use 4 IO Card
    four_IO_card0.writeRelay(color,state);
    delay(100);
  }
}
/*
  @brief Determines the state of stacklights given a Status byte.  
*/
void manageStackLight(byte systemStatus, byte oven) {

  switch (systemStatus) {
    case STARTUP:   //Oven below tempreture setpoint
      manageRelays(oven, BUZZ, LOW);      
      manageRelays(oven, GRN, LOW);      
      manageRelays(oven, ORG, LOW);     
      manageRelays(oven, RED, HIGH);
      break;
    case OVEN_READY:  //Oven has reached tempreture setpoint
      manageRelays(oven, BUZZ, LOW);     
      manageRelays(oven, GRN, HIGH);     
      manageRelays(oven, ORG, LOW);     
      manageRelays(oven, RED, LOW);
      break;
    case CYCLE_ACTIVE:
      manageRelays(oven, BUZZ, LOW);     
      manageRelays(oven, GRN, LOW);    
      manageRelays(oven, ORG, HIGH);    
      manageRelays(oven, RED, LOW);
      break;
    case TIME_COMPLETE:      
      manageRelays(oven, ORG, LOW);     
      manageRelays(oven, RED, LOW);     
      if (StackLightOnTimer[oven].checkTimer()==0) {  //Time comlete
        manageRelays(oven, GRN, LOW);           //Turn off the stack light                                 
        manageRelays(oven, BUZZ, LOW);          //Turn off the stack light
        StackLightOffTimer[oven].startTimer(500);    //Off duration
        break;
      }

      if (StackLightOffTimer[oven].checkTimer()==0) {   //Time copmlete
        manageRelays(oven, GRN, HIGH);            //Turn ON the stack light
        manageRelays(oven, BUZZ, buzzerMode.Value());   //Turn ON the stack light if buzzerMode is true.
        StackLightOnTimer[oven].startTimer(1000);    
        break;
      } 

      break;
    case ACKNOWLEDGED:
      // No action required
      manageRelays(oven, BUZZ, LOW);
      manageRelays(oven, GRN, HIGH);
      manageRelays(oven, ORG, LOW);
      manageRelays(oven, RED, LOW);
      break;
    case TEMPERATURE_DATA_SAVED:
      // No action required
      break;
    case ERROR:
      manageRelays(oven, RED, HIGH);
      manageRelays(oven, BUZZ,  HIGH);
      break;
  }
}

/*
 * @brief Displays remaing Time to the LCD
 * @param oven The oven we are displaying time and status for, 0-7.
*/
void displayRemainingTime(int oven) {
  
    lcd.setCursor(10, oven);    //Column, Row
    lcd.print("   ");           //Clear the remaining time.
    lcd.setCursor(10, oven);    //Column, Row
    lcd.print(timer[oven].remainingTime()/60000);   //converts to minutes
    lcd.setCursor(14, oven);    //Column, Row
    lcd.print("Min");
    lcd.setCursor(19, oven);    //Column, Row
    lcd.print(status[oven]);
  return;
}
 

/*
@brief
Displays a temperature reading from myData.Temps[0]
@param row - Row on the LCD screen to display the temperature to. 
*/
void Display4Temps(int row) {
  String Temp_Disp;
  Temp_Disp = String(myData.OvenID) + String(": ") + String((float)myData.Temps[0]/10,1);
  lcd.setCursor(0, row);      // set cursor to first column, first row
  lcd.print(Temp_Disp);
  lcd.write((byte)0);       //Degree Symbol
  lcd.print("C");
  // lcd.setCursor(9, j);
  // lcd.print("|");
  return;
  
}


/*
@brief
Recieves an array of 8 tempreture readings
and displays them on an LCD screen.  
*/
void Display8Temps(int x[]) {
  String Temp_Disp;
  //lcd.clear();
  // set cursor to first column, first row
  Temp_Disp = String("1: ") + String(x[0]/10);
  lcd.setCursor(0, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 0);
  lcd.print("C");

  Temp_Disp = String("2: ") + String(x[1]/10);
  lcd.setCursor(0, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 1);
  lcd.print("C");
  
  Temp_Disp = String("3: ") + String(x[2]/10);
  lcd.setCursor(0, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 2);
  lcd.print("C");

  Temp_Disp = String("4: ") + String(x[3]/10);
  lcd.setCursor(0, 3);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 3);
  lcd.print("C");

  Temp_Disp = String("5: ") + String(x[4]/10);
  lcd.setCursor(10, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 0);
  lcd.print("C");

  Temp_Disp = String("6: ") + String(x[5]/10);
  lcd.setCursor(10, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 1);
  lcd.print("C");

  Temp_Disp = String("7: ") + String(x[6]/10);
  lcd.setCursor(10, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 2);
  lcd.print("C");

  Temp_Disp = String("8: ") + String(x[7]/10);
  lcd.setCursor(10, 3);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 3);
  lcd.print("C");
  
  return;
}


// Update the oven ID, temperature setpoint, cook time, or data interval time based on the current menu option and the rotary encoder
// Does not update the transmitted data packet.
void updateMenuValue() {
  lcd.resetEncoder();
  int rotaryValue = -lcd.readEncoder();
  delay(10);
  
  switch (currentMenuOption) {
    case OVEN_ID:
      ovenID.setValue(ovenID.Value() + rotaryValue);
      break;
    case TEMPERATURE_SETPOINT:
      temperatureSetpoint.setValue(temperatureSetpoint.Value() + rotaryValue * 5.00);
      break;
    case COOK_TIME:
      cookTime.setValue(cookTime.Value() + rotaryValue);
      break;
    case DATA_INTERVAL_TIME:
      dataIntervalTime.setValue(dataIntervalTime.Value() + rotaryValue);
      break;
    case NUM_TC_PER_OVEN:
      numTCperOven.setValue(numTCperOven.Value() + rotaryValue);
      break;
    case NUM_OVENS:
      numOvens.setValue(numOvens.Value() + rotaryValue);
      break;
    case BUZZER_MODE:
      buzzerMode.setValue(buzzerMode.Value() + rotaryValue);
      break;
    case TC_CALI_MODE:
      tc_cali_mode.setValue(tc_cali_mode.Value() + rotaryValue);
      break;
    case TC_CALI: 
      TC_Offsets[tc_cali_mode.Value()] += rotaryValue;
      break;
  }
}



// Display the current menu option
void displayMenuOption() {
  // lcd.clear();
    switch (currentMenuOption) {
      case HOME:
        //Update the remaining time once every 250ish miliseconds.
        //Maybe move this elsewhere?

        //for Display4Temps
        if (numTCperOven.Value() == 1){
          if (millis() % 250 < 50) {
            displayRemainingTime(0);
            displayRemainingTime(1);
            displayRemainingTime(2);
            displayRemainingTime(3);
          }
        }

        // Display6Temps(myData.Temps);
        //Prints the remaining cook time to the LCD Screen
        // displayRemainingTime(0);
        break;
      case OVEN_ID:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);    //Column, Row
        lcd.print("Oven ID");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(ovenID.Value());
        lcd.print(" ");         //Erases last digit when going from 10 to 9  
        break;
      case TEMPERATURE_SETPOINT:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Temperature Setpoint");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(temperatureSetpoint.Value());
        lcd.write((byte)0);       //Degree Symbol
        lcd.print("C ");
        break;
      case COOK_TIME:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Cook Time:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(cookTime.Value());
        lcd.print(" Minutes ");
        break;
      case DATA_INTERVAL_TIME:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Data Interval Time:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(dataIntervalTime.Value());
        lcd.print(" Seconds ");
        break;
      case NUM_TC_PER_OVEN:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Num T.C's per Oven:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(numTCperOven.Value());
        lcd.print(" ");
        break;
      case NUM_OVENS:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Number of Ovens:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(numOvens.Value());
        lcd.print(" ");
        break;
      case BUZZER_MODE:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Buzzer On/ Off:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(buzzerMode.Value());
        break;
      case TC_CALI_MODE:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("TC Calibration:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(tc_cali_mode.Value());
        break;
      case TC_CALI:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("TC Calibration:");
        lcd.setCursor(0, 1);
        lcd.print("Temperature ");
        lcd.print(tc_cali_mode.Value());
        lcd.print(":");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print((TC_Card.readTemp(tc_cali_mode.Value()))+(float)TC_Offsets[tc_cali_mode.Value()]/10);   //Use the rotery encoder to hone in the temp.
        lcd.write((byte)0);       //Degree Symbol
        lcd.print("C ");
        break;
      case SAVE_PARAM:
        tc_cali_mode.setValue(0);     //Reset the calibration mode value
        saveParametersToSD();
        lcd.setCursor(0, 1);
        currentMenuOption = MenuOption(0);    //Returns to menuOption Home
        displayMenuOption();
        break;

    }
    updateMenuValue();

}

// Update the current menu option on each button press.
void updateMenuOption() {  
  if(lcd.readButtonLatch(1)){   //checks for a single button press and release.
    lcd.clear();
    
    if (currentMenuOption==TC_CALI){
      
      if (tc_cali_mode.Value()==8){   //If we have looped thru all possible TCs
          tc_cali_mode.setValue(0);
          currentMenuOption = SAVE_PARAM;
        }
      tc_cali_mode.setValue(tc_cali_mode.Value()+1);    //go to the next TC calibration
      return;
    }
    currentMenuOption = MenuOption((currentMenuOption + 1 + bool(tc_cali_mode.Value())) % Num_Menu_Screens);  //Somehow % makes sure we don't go to a non existant menu
  }
}



// === Stuff for reading data from PC=== //
// https://forum.arduino.cc/t/demo-of-arduino-control-with-a-python-gui-program/261844
byte servoMin = 10;      //This servo stuff can be deleted/ renamed later.  Used for testing .py to ESP comm
byte servoMax = 170;
byte servoPos = 0;
byte newServoPos = servoMin;

const byte numLEDs = 2;
byte ledPin[numLEDs] = {12, 13};
byte ledStatus[numLEDs] = {0, 0};

const byte buffSize = 40;
char inputBuffer[buffSize];
const char startMarker = '<';
const char endMarker = '>';
byte bytesRecvd = 0;
boolean readInProgress = false;
boolean newDataFromPC = false;

char messageFromPC[buffSize] = {0};





//=============
/*
@brief Saves the Py-ESP transmistion to the OMT parameters
*/
void parseConfigData() {
// assumes the data will be received as (eg) 0,1,35
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(inputBuffer,","); // get the first part
  ovenID.setValue(atoi(strtokIndx));    //  convert to an integer 
  strtokIndx = strtok(NULL, ",");       // this continues where the previous call left off
  temperatureSetpoint.setValue(atoi(strtokIndx));  
  strtokIndx = strtok(NULL, ",");    
  cookTime.setValue(atoi(strtokIndx));
  strtokIndx = strtok(NULL, ","); 
  dataIntervalTime.setValue(atoi(strtokIndx));
  strtokIndx = strtok(NULL, ","); 
  numTCperOven.setValue(atoi(strtokIndx));
  strtokIndx = strtok(NULL, ","); 
  numOvens.setValue(atoi(strtokIndx));
  strtokIndx = strtok(NULL, ","); 
  buzzerMode.setValue(atoi(strtokIndx));
  strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[1] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[2] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[3] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[4] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[5] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[6] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[7] = atoi(strtokIndx);
  // strtokIndx = strtok(NULL, ","); 
  // TC_Offsets[8] = atoi(strtokIndx);

  // cycleNum.SetValue(atoi(strtokIndx));
  ledStatus[0] = ovenID.Value()-1;    //Temporay for testing.
  Serial.print("numOvens.Value(): ");
  Serial.println(numOvens.Value());       //Simple check that the data was transmitted and recieved correctly.
  //Implement a responce back to the .py for automatic error checking

  saveParametersToSD();   //Save the new parameters to the SD card.
}

// Temporary for checking serial data com from ESP to Python.
void replyToPC() {
  if (newDataFromPC) {
    newDataFromPC = false;
    Serial.print("<LedA, ");
    Serial.print(ledStatus[0]);
    Serial.print(", LedB, ");
    Serial.print(ledStatus[1]);
    Serial.print(", SrvPos, ");
    Serial.print(newServoPos);
    Serial.print(", Time, ");
    Serial.print(millis() >> 9); // divide by 512 is approx = half-seconds
    Serial.println(">");
  }
}

/*
@brief receives data from PC and saves it into inputBuffer
*/
void getDataFromPC() {    
  // Serial.println("<In getDataFromPC>");
  if(Serial.available() > 0) {
    if(debug){Serial.println("<Serial.available>");}
    char x = Serial.read();
    // the order of these IF clauses is significant
    if (x == endMarker) {
      readInProgress = false;
      newDataFromPC = true;
      inputBuffer[bytesRecvd] = 0;
      parseConfigData();
      replyToPC();    //Send some data back to the python script. 
    }
    if(readInProgress) {
      inputBuffer[bytesRecvd] = x;
      bytesRecvd ++;
      if (bytesRecvd == buffSize) {
        bytesRecvd = buffSize - 1;
      }
    }

    if (x == startMarker) { 
      bytesRecvd = 0; 
      readInProgress = true;
    }
  }
}



void setup() {
  //Eliminates errors when using an external 5V PSU.  
  //Not Necessary when using USB
  delay(1000);    
  // Init Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //===LCD SCREEN===//
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  //Set the backlight
  lcd.writeBl(20);
  lcd.clear();
  // create a new character
  lcd.createChar(0, degree);

  //===Boot Screen===//
  lcd.clear();
  lcd.setCursor(0, 0);    //Column, Row
  lcd.print("Oven Monitor Timing");
  delay(200);
  lcd.setCursor(0, 1);    
  lcd.print("Corry Rubber Co");
  delay(200);
  lcd.setCursor(0, 2);    
  lcd.print("Jake's Custom Shop");
  delay(200);
  lcd.setCursor(0, 3);    
  lcd.print("Version 1.2");
  if (debug2) (2500);
  lcd.clear();

  //===SD Card Stuff==//
  if(!SD.begin(SM_ESP32PI_SDCARD_CS)){
    Serial.println("Card Mount Failed");
    lcd.setCursor(0, 0);    //Column, Row
    lcd.print("Card Mount Failed");           //Clear the remaining time.
    if (debug2) delay(3000);
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    lcd.setCursor(0, 1);    //Column, Row
    lcd.print("No SD card attached");           //Clear the remaining time.
    if (debug2) delay(3000);
  }
  
  //Reads the parameter file on the SD card.
  //also asigns read values to the Parameters class
  readParameterFile(SD, "/parameters.txt");   
  myData.OvenID = ovenID.Value();     //Please fix this

  //Create a file for each oven's tempreture data.
  for (int i=ovenID.Value(); i<numOvens.Value(); i++){
    newDatafile(i);
  }



  //===4Relay 4 HV===// 
  if (four_IO_card0.begin() ){
    Serial.print("Four Relays four HV Inputs Card detected\n");
    four_IO_card0.writeRelay(LOW);
  }
  else{
    Serial.print("Four Relays four HV Inputs Card NOT detected!\n");
  }
  
  //===8Relay stack level 2===//
  if (0 == eight_relay_card2.begin(2) )
  {
    Serial.print("Eight Relays Card detected\n");
    Use8RelayCard = true;
    // eight_relay_card2.writeChannel(LOW);
  }
  else{
    Serial.print("Eight Relays Card NOT detected!\n");
    lcd.setCursor(0, 3);    //Column, Row
    lcd.print("8Relay card 2 Error");           //Clear the remaining time.
    if (debug2) delay(3000);
    Use8RelayCard = false;
  }
  //===8Relay stack level 3===//
  if (0 == eight_relay_card3.begin(3) )   //.begin() returns 0 if card is detected. (This is backwards from other SM cards)
  {
    Serial.print("Eight Relays Card detected\n");
    // eight_relay_card3.writeChannel(LOW);
  }
  else
  {
    Serial.print("Eight Relays Card NOT detected!\n");
    lcd.setCursor(0, 3);    //Column, Row
    lcd.print("8Relay card 3 Error");           //Clear the remaining time.
    if (debug2) delay(3000);
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  }

  //TC Hat check.
  if (TC_Card.begin() ){
    Serial.print("TC HAT detected\n");
    TC_Check=true;
    TC_Card.setType(1,2);   //Chanel, type J=2
    TC_Card.setType(2,2);
    TC_Card.setType(3,2);
    TC_Card.setType(4,2);
    TC_Card.setType(5,2);
    TC_Card.setType(6,2);
    TC_Card.setType(7,2);
    TC_Card.setType(8,2);

  }
  else{
    Serial.print("TC HAT NOT detected!\n");
    lcd.setCursor(0, 2);    //Column, Row
    lcd.print("TC HAT NOT detected!");           //Clear the remaining time.
    if (debug2) delay(3000);
    TC_Check=false;
    dataIntervalTime.setValue(1);
  }



  dataIntervalTimer.startTimer(dataIntervalTime.Value()*1000); //Start a timer to send data packets at specified intervals.
  // for (int i=0; i++; i<4){   //Had an issue with this for some reason.
  StackLightOffTimer[0].startTimer(1);
  StackLightOnTimer[0].startTimer(1);
  StackLightOffTimer[1].startTimer(1);
  StackLightOnTimer[1].startTimer(1);
  StackLightOffTimer[2].startTimer(1);
  StackLightOnTimer[2].startTimer(1);
  StackLightOffTimer[3].startTimer(1);
  StackLightOnTimer[3].startTimer(1);
  // }

  //So the screen atleast displays somthing
  lcd.clear();
  for(int i=0;i<5;i++) {
    Display4Temps(i);
  }

  // for debugging of Python to ESP data transmission.
  if(!TC_Check){    //If the TC card is not detected, assume I am debugging with a esp32doit-devkit-v1
    pinMode(ledPin[0], OUTPUT);
    pinMode(ledPin[1], OUTPUT);
    digitalWrite(ledPin[0], HIGH);
    digitalWrite(ledPin[1], HIGH);
    delay(1000);
    digitalWrite(ledPin[0], LOW);
    digitalWrite(ledPin[1], LOW);
  }
}


// For debugging.  Only called if TC_check is false
void updateLEDs(){
  digitalWrite(ledPin[0], ledStatus[0]);
  digitalWrite(ledPin[1], ledStatus[1]);
}
 


void loop() {
 
  //===COOK TIMING SYSTEM===// 
  //Continusuly checks each of 6 possible ovens for timer complete.
  //Some Transmitters will have multiple Ovens attached.
  for (int j = 0; j < numOvens.Value(); j++) {
    myData.OvenID=ovenID.Value()+j;     //For systes with multiple ovens, assing myData.ovenID a unique value. 
                                        //This allows the PC data log to distingush between different ovens
                                        //connected to the same transmitter.

    //===Handle Start/Stop Button on for timer[i]===//
    //Should be compatible with up to 4 Start/Stop buttons for 4 ovens.
    // if (lcd.readButtonLatch(j+2)){   //for use with LCD screen buttons
    if (four_IO_card0.readOptoAC(j+1) || lcd.readButtonLatch(j+2)) {        //Ideal if we can create a readButtonLatch version for relay card
      while(four_IO_card0.readOpto(j+1)){       //Wait for the button to be released. SM IO are indexed at 1
        delay(100);
      }
      delay(500);         //Holy cow this elimiates alot of state issues. 
      cycleNum[j]+=1;     //Count cooking cycles for each oven j.

      if (debug){
        Serial.print("Button Number: ");
        Serial.println(j);
      }

     //This switch determines what to do after the Start/Stop button is pressed
      switch(status[j]) {
        case STARTUP:                   //Pre-heating
        case OVEN_READY:                //Ready
        case ACKNOWLEDGED:              //Acknowledged, Ready, or Pre-heating  
          timer[j].startTimer(cookTime.Value()*60000); //Start the timer.  cookTime is units of minutes.
          status[j] = CYCLE_ACTIVE;
          Serial.print("Timer Started for: ");
          Serial.println(cookTime.Value());
          newDatafile(j);    //Create a new .csv data file for this oven's new cycle.  Applies csv headers.
          //Flashing Green. No buzzer
          break;
        
        //Turn off the buzzer/ flashy lights
        case CYCLE_ACTIVE: //Active cycle OR
        case TIME_COMPLETE: //Time Complete
          //Turn off buzzer and Inidcator light
          status[j] = ACKNOWLEDGED;    //Acknowledge (GUI Stops collecting data)
          Serial.println("Acknowledged (GUI Stops collecting data)");
          timer[j].startTimer(0);  //Stop the Timer if it is active.
          break;
        default:
          break;
      }
    }

    if(!timer[j].checkTimer()){  //When the cooking cycle is complete
      status[j] = TIME_COMPLETE;    //Time Complete
      //Two short Beeps and Two short Green Flashes
      Serial.println("Cooking cycle is complete");
      Serial.println("Waiting for Acknowledgement.");
    }   
  }

  //===DATA TRANSMITTION===//
  //Runs only when the transmission interval has completed.
  //Loops for each connected Oven and each TC on each oven.
  if (!dataIntervalTimer.checkTimer()) {   //When send interval is complete.
    for (int j = 0; j< numOvens.Value(); j++){
      myData.OvenID=ovenID.Value()+j;     //For systes with multiple ovens, assiging myData.ovenID a unique value. 
                                          //This allows the PC data log to distingush between different ovens
                                          //connected to the same transmitter.
      
      myData.Count ++;    //For error checking.  
      //Can implement a check on the reciever and PC to make sure no transmissions are lost.  
      if (debug) {
        Serial.print(myData.OvenID);                      
        Serial.print(", ");
        Serial.print(myData.Count);                      
        Serial.print(", ");
      }
      for(int i = 0; i < (numTCperOven.Value()); i++){
        if (TC_Check){
          myData.Temps[i] = TC_Card.readTemp(i+j+1)*10 + TC_Offsets[i+j+1];    //TC_Card.readTemp wants 1 thru 8.  Mult by 10 to increase precision. 
          //Tempreture checking
            if (myData.Temps[i] < temperatureSetpoint.Value()*10)
              status[j] = STARTUP;                      //Oven temp below setpoint.  
            else {
              if (status[j]==STARTUP)                   //Oven temp above setpoint and in mode startup
                status[j] = OVEN_READY;                 //Set the status to OVEN_READY
            }
        }
        else
          myData.Temps[i] = (random(15,25)+0.1)*10 + TC_Offsets[i+j+1];   //Adding 0.1 to test tenth digit precision.
      }


      //Asign unused myData.Temps to zero
      for(int i = numTCperOven.Value(); i < numTCperOven.getMaxVal(); i++){
        myData.Temps[i] = 0;
      }     

      //Build the myData transmission packet
      myData.Status = status[j];
      // Send message via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      if (result == ESP_OK && debug)
        Serial.println("Sent with success using ESP-Now");
      else
        Serial.println("Error sending the data");
      dataIntervalTimer.startTimer(dataIntervalTime.Value()*1000);   //Restart the timer

    
      String myData_String = SerialSendData(myData);           //Send data over USB Serial Port.  Sends a packet for ea. oven
      
      //Build a file_name but not a new file.  This must be the same structure as is used for newDataFile();
      String file_name = "/"+String(myData.OvenID) + "-sampleData-" + String(cycleNum[j]) +".txt";
      appendFile(SD, file_name, myData_String);   //Adds the data packet from SerialSendData to the SD card file
      
      //I don't like it but it seems to work.  
      if(currentMenuOption==HOME){
        Display4Temps(j);    
      }

    }//Loop for each oven
  } //Data Send interval 
  
  
  
  //Check each oven to see if the status has been updated.
  //Status == TIME_COMPLETE blinks the GRN light and thus requries more calls to mangeSatckLight().
  for (int j = 0; j< numOvens.Value(); j++){
    if (prev_status[j] != status[j] || status[j] == TIME_COMPLETE) {
      manageStackLight(status[j], j);  //If the system status is updated, update the stacklight. 
      prev_status[j] = status[j];
    }
  }
  
  displayMenuOption();              //Display the current menu option on the LCD screen. Updates countdown timer. And Displays temperature
  updateMenuOption();               // Update the current menu option
  updateMenuValue();                // Update the menu value based on the current menu option and the rotary encoder

  // Check for data seny from python to ESP-32
  getDataFromPC();
  updateLEDs();
  

}   //Continuous Loop As Fast As Possible




