/*!
    Jake Lehotsky
    Jake's Custom Shop, LLC
    August, 2023

    Built for Sequent Microsystems ESP-Pi 
    Reads Tempature data from up to 8 RTDs on the Sequent Microsystems RTD HAT.
    Transmits Tempature data to a reciever via ESP_now wireless protocall.
    Displays 6  values to a 20x4 Sequent Microsystem LCD adapter HAT.

    Notes: 
     - Must use 3-Wire RTD or jumper on RTD DAQ. inputs
     - For use with Reciever3.ino
     - Will work with a plain ol' ESP32 for testing purposes.  
     - Adding a menu option requres update to updateMenuOption() 

     
*/


/*
TODO:
  Necessary Changes for impementation
     - Use external buttons to start stop timers!
*/


#include "Timer.h"    //Custom Header and C++ file for the Timing System
#include <esp_now.h>
#include <WiFi.h>
#include "SM_RTD.h"
#include "SM_LCDAdapter.h"
#include "LCD_Menu.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "SM_ESP32Pi.h"
#include "SM_4REL4IN.h"
SM_LCDAdapter lcd = SM_LCDAdapter();

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0x94,0xB9,0x7E,0xF9,0x3A,0x18};

SM_RTD card(0);     //RTD Data Acquisition HAT with stack level 0 (no jumpers installed)
SM_4REL4IN IO_Card(1); //Four Relays four HV Inputs HAT with stack level 1 (1 Jmper Installed)



//int debug = 0;    //Turns off addational serial print fuctions.
int debug = 1;    //Basic Debuging.  Enables random numbers for Tempeture values if RTD hat is not detected.
bool RTD_Check;   //Check to see if the Sequent RTD hat exists.

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int OvenID = 1; // must be unique for each sender board. 
    int Count = 0;    //Counts the number of transmissions.
    int Temps[8];   //Each element is a seperate chamber in the oven?
    int Status = 0;    //
    //int BatchID;     //6-Digit identification number for each batch. 
} struct_message;
struct_message myData;        // Create a struct_message called myData

enum SystemStatus {
  STARTUP,
  OVEN_READY,
  CYCLE_ACTIVE,
  TIME_COMPLETE,
  ACKNOWLEDGED,
  TEMPERATURE_DATA_SAVED,
  ERROR
};
// Define the current menu option
//SystemStatus myData.Status = STARTUP;

enum StackLight {
  OFF,
  BUZZ,
  GRN,
  ORG,
  RED
};




esp_now_peer_info_t peerInfo; // Create peer interface



//===Initilize timers for tracking cook time===//
//A timer interval of 0 disables the timer
Timer timer[6];
//int CookTime[6] = {7000,2000,0,0,0,0};    Changed to class Parameter
//Create a timer for sending data packets every XX miliseconds z
Timer dataIntervalTimer;
Timer StackLightOnTimer;
Timer StackLightOffTimer;



// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 



void setup() {
   
  // Init Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //===SD Card Stuff==//
  if(!SD.begin(SM_ESP32PI_SDCARD_CS)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  //Reads the parameter file on the SD card.
  //also asigns read values to the Parameters class
  readParameterFile(SD, "/parameters.txt");   
  myData.OvenID = ovenID.Value();     //Please fix this

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  if (card.begin() )
  {
    Serial.print("Four Relays four HV Inputs Card detected\n");
  }
  else
  {
    Serial.print("Four Relays four HV Inputs Card NOT detected!\n");
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
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
    return;
  }

  //RTD Hat check.
  if (card.begin() ){
    Serial.print("RTD HAT detected\n");
    RTD_Check=1;
  }
  else{
    Serial.print("RTD HAT NOT detected!\n");
    RTD_Check=0;
  }

  //===LCD SCREEN===//
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  lcd.writeBl(20);

  dataIntervalTimer.startTimer(dataIntervalTime.Value()*1000); //Start a timer to send data packets at specified intervals.
  StackLightOffTimer.startTimer(1);
  StackLightOnTimer.startTimer(1);
}


 
void loop() {
 
  //===TIMING SYSTEM===//
  //Check each of 6 possible timers.
  //Some Transmitters will have multiple Ovens attached.
  for (int i = 1; i <= 5; i++) {
    
    //Check each of 6 buttons for a press
    //This will need updated once physical buttons are implmented
    if (lcd.readButtonLatch(i+1)){
      switch(myData.Status) {
        //Start the timer
        case STARTUP:   //Pre-heating
        case OVEN_READY:   //Ready
        case ACKNOWLEDGED:   //Acknowledged, Ready, or Pre-heating  
          timer[i].startTimer(cookTime.Value()*1000); //Start the timer.  cookTime is units of seconds.
          myData.Status = 2;
          Serial.print("Timer Started for: ");
          Serial.println(cookTime.Value());
          //Flashing Green. No buzzer
          break;
        
        //Turn off the buzzer/ flashy lights
        case CYCLE_ACTIVE: //Active cycle OR
        case TIME_COMPLETE: //Time Complete
            //Turn off buzzer and Inidcator light
            myData.Status = ACKNOWLEDGED;    //Acknowledge (GUI Stops collecting data)
            Serial.println("Acknowledged (GUI Stops collecting data)");
            timer[i].startTimer(0);  //Stop the Timer if it is active.
            break;
         default:
          break;
      }
    }
    
    if(!timer[i].checkTimer()){  //When the cooking cycle is complete
        myData.Status = TIME_COMPLETE;    //Time Complete
        //Two short Beeps and Two short Green Flashes
        Serial.println("Cooking cycle is complete");
        Serial.println("Waiting for Acknowledgement.");
      }
    
  }


  //===DATA TRANSMITTION===//
  if (!dataIntervalTimer.checkTimer()) {   //When send interval is complete.
  	
    myData.Count ++;    //For error checking.  
    //Can implement a check on the reciever and PC to make sure no transmissions are lost.  
    if (debug) {
      Serial.print(myData.OvenID);                      
      Serial.print(", ");
      Serial.print(myData.Count);                      
      Serial.print(", ");
    }

    //===TEMPERATURE CHECKING===//
    for(int i = 0; i < 8; i++){
      if (RTD_Check && !debug)
        myData.Temps[i] = card.readTemp(i+1);   //card.readTemp wants 1 thru 8
      else
        myData.Temps[i] = random(0,50);
      Serial.print(myData.Temps[i]);
      Serial.print(", ");
    }
    Serial.println();
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
     
    if (result == ESP_OK)
      Serial.println("Sent with success");
    else
      Serial.println("Error sending the data");
    dataIntervalTimer.startTimer(dataIntervalTime.Value());   //Restart the timer
  } //Data Send interval  

  // Display the current menu option on the LCD screen
  displayMenuOption();

  // Update the current menu option
  updateMenuOption();

  // Update the menu value based on the current menu option and the rotary encoder
  updateMenuValue();

  manageStackLight(myData.Status);
  // beepAndFlash(BUZZ, 1000);

}


void manageStackLight(int systemStatus) {

  switch (systemStatus) {
    case STARTUP:
      IO_Card.writeRelay(BUZZ, LOW);
      IO_Card.writeRelay(GRN, LOW);
      IO_Card.writeRelay(ORG, LOW);
      IO_Card.writeRelay(RED, HIGH);
      break;
    case OVEN_READY:
      // IO_Card.writeRelay(BUZZ, LOW);
      // IO_Card.writeRelay(GRN, HIGH);
      IO_Card.writeRelay(ORG, LOW);
      IO_Card.writeRelay(RED, LOW);
      // beepAndFlash(GRN, OFF, 1000);
      break;
    case CYCLE_ACTIVE:
      IO_Card.writeRelay(BUZZ, LOW);
      IO_Card.writeRelay(GRN, LOW);
      IO_Card.writeRelay(ORG, HIGH);
      IO_Card.writeRelay(RED, LOW);
      break;
    case TIME_COMPLETE:

      IO_Card.writeRelay(ORG, LOW);
      IO_Card.writeRelay(RED, LOW);
      if (StackLightOnTimer.checkTimer()==0) {    //Time comlete
        IO_Card.writeRelay(GRN, LOW);           //Turn off the stack light
        IO_Card.writeRelay(BUZZ, LOW);          //Turn off the stack light
        StackLightOffTimer.startTimer(1000);    //Off duration
      }

      if (StackLightOffTimer.checkTimer()==0) {   //Time copmlete
        IO_Card.writeRelay(GRN, HIGH);            //Turn ON the stack light
        IO_Card.writeRelay(BUZZ, HIGH);           //Turn ON the stack light
        StackLightOnTimer.startTimer(2000);    
      } 

      break;
    case ACKNOWLEDGED:
      // No action required
      IO_Card.writeRelay(BUZZ, LOW);
      IO_Card.writeRelay(GRN, HIGH);
      IO_Card.writeRelay(ORG, LOW);
      IO_Card.writeRelay(RED, LOW);
      break;
    case TEMPERATURE_DATA_SAVED:
      // No action required
      break;
    case ERROR:
      IO_Card.writeRelay(RED, HIGH);
      IO_Card.writeRelay(BUZZ, HIGH);
      break;
  }
}



//Displaies Remaing Time to the LCD
void displayRemainingTime() {
  if (millis() % 500 < 50) {
      lcd.setCursor(0, 3);    //Column, Row
      lcd.print(timer[1].remainingTime()/1000);
      lcd.print(" Sec");
      lcd.setCursor(8, 3);    //Column, Row
      lcd.print("| Status: ");
      lcd.print(myData.Status);

  } 
}
 
/*
Recieves an array of tempreture readings (up to 6)
and displays them on an LCD screen.  
*/
void DisplayTemps(int x[]) {
  //String Temp_Disp = String(x);
  String Temp_Disp;
  //lcd.clear();
  // set cursor to first column, first row
  Temp_Disp = String("1: ") + String(x[0]);
  lcd.setCursor(0, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 0);
  lcd.print("C");

  Temp_Disp = String("2: ") + String(x[1]);
  lcd.setCursor(0, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 1);
  lcd.print("C");
  
  Temp_Disp = String("3: ") + String(x[2]);
  lcd.setCursor(0, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(7, 2);
  lcd.print("C");

  Temp_Disp = String("4: ") + String(x[3]);
  lcd.setCursor(10, 0);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 0);
  lcd.print("C");

  Temp_Disp = String("5: ") + String(x[4]);
  lcd.setCursor(10, 1);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 1);
  lcd.print("C");

  Temp_Disp = String("6: ") + String(x[5]);
  lcd.setCursor(10, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 2);
  lcd.print("C");
  
  return;
}


// Display the current menu option
void displayMenuOption() {
  // lcd.clear();
    switch (currentMenuOption) {
      case HOME:
        DisplayTemps(myData.Temps);
        //Prints the remaining cook time to the LCD Screen
        displayRemainingTime();
        break;
      case OVEN_ID:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);    //Column, Row
        lcd.print("Oven ID");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(ovenID.Value());
        break;
      case TEMPERATURE_SETPOINT:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Temperature Setpoint");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(temperatureSetpoint.Value());
        lcd.print(" Degrees C");
        break;
      case COOK_TIME:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Cook Time");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(cookTime.Value());
        lcd.print(" Minutes");
        break;
      case DATA_INTERVAL_TIME:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Data Interval Time");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(dataIntervalTime.Value());
        lcd.print(" Seconds");
        break;
      case NUM_CHANNELS:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Number of tempreture probes");
        lcd.setCursor(0, 2);
        lcd.print(numChannels.Value());
        break;
      case SAVE_PARAM:
        String parameters = "";
        parameters += "ovenID: " + String(ovenID.Value()) + "\n";
        myData.OvenID = ovenID.Value();     //Please fix this
        parameters += "temperatureSetpoint: " + String(temperatureSetpoint.Value()) + "\n";
        parameters += "cookTime: " + String(cookTime.Value()) + "\n";
        parameters += "dataIntervalTime: " + String(dataIntervalTime.Value()) + "\n";
        parameters += "numChannels: " + String(numChannels.Value()) + "\n";
        writeFile(SD, "/parameters.txt", parameters);
        readParameterFile(SD, "/parameters.txt");
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
    currentMenuOption = MenuOption((currentMenuOption + 1) % 7);  //Somehow % makes sure we don't go to a non existant menu
  }
}

// Update the oven ID, temperature setpoint, cook time, or data interval time based on the current menu option and the rotary encoder
void updateMenuValue() {
  lcd.resetEncoder();
  int rotaryValue = -lcd.readEncoder(); 
  switch (currentMenuOption) {
    case OVEN_ID:
      ovenID.setValue(ovenID.Value() + rotaryValue);
      break;
    case TEMPERATURE_SETPOINT:
      temperatureSetpoint.setValue(temperatureSetpoint.Value() + rotaryValue * 10.0);
      break;
    case COOK_TIME:
      cookTime.setValue(cookTime.Value() + rotaryValue);
      break;
    case DATA_INTERVAL_TIME:
      dataIntervalTime.setValue(dataIntervalTime.Value() + rotaryValue);
      break;
    case NUM_CHANNELS:
      numChannels.setValue(numChannels.Value() + rotaryValue);
      break;
  }
}

//===SD Card Stuff===//
void writeFile(fs::FS &fs, const char * path, String message){
  if (debug)Serial.printf("Writing file: %s\n", path);

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
  // return;
}

void appendFile(fs::FS &fs, const char * path, String message){
  if (debug)Serial.printf("Appending to file: %s\n", path);

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
  if (debug)Serial.printf("Reading file: %s\n", path);

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
    } else if (parameterName == "numChannels") {
      numChannels.setValue(intValue);
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
    Serial.print("numChannels: ");
    Serial.println(numChannels.Value());
    
  }


  file.close();
}