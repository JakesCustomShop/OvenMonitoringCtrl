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

     
*/


/*
TODO:
  Necessary Changes for impementation
   - Add SD card data logging.
      - Add file naming for SD card data saving?
   - Implement tempreture monitoring incase of low-temp.
   - Make stack light timing system for each connected oven.
*/


#include "Timer.h"    //Custom Header and C++ file for the Timing System
#include <esp_now.h>
#include <WiFi.h>

//#include "SM_RTD.h"
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


SM_4REL4IN four_IO_card0(0); //Four Relays four HV Inputs HAT with stack level 0 (no Jmper Installed)
SM_TC TC_Card(1);     //TC Data Acquisition HAT with stack level 1 (no jumpers installed)
SM_8relay eight_relay_card2 = SM_8relay() ; //Eight Relays HAT with stack level 2
SM_8relay eight_relay_card3 = SM_8relay(); //Eight Relays HAT with stack level 3 
// TC_Card.setType(1,TC_TYPE_J);   //Specify the Thermocouple type

//int debug = 0;    //Turns off addational serial print fuctions.
int debug = 1;    //Basic Debuging.  Enables random numbers for Tempeture values if TC hat is not detected.
bool TC_Check;   //Check to see if the Sequent TC hat exists.
int status[8] = {0,0,0,0,0,0,0,0};   //Status byte for each oven.
int prev_status[8] = {1,1,1,1,1,1,1,1};   //Previous status for determinging when to update a stack light. Anything except STARTUP?

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
//int CookTime[6] = {7000,2000,0,0,0,0};    Changed to class Parameter
//Create a timer for sending data packets every XX miliseconds z
Timer dataIntervalTimer;
//millis() based timer for blinking the stack lights. 
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

//4Relay 4 HV
  if (four_IO_card0.begin() )
  {
    Serial.print("Four Relays four HV Inputs Card detected\n");
    four_IO_card0.writeRelay(LOW);
  }
  else
  {
    Serial.print("Four Relays four HV Inputs Card NOT detected!\n");
  }
  //8Relay stack level 2
  if (eight_relay_card2.begin(2) )
  {
    Serial.print("Four Relays four HV Inputs Card detected\n");
    // eight_relay_card2.writeChannel(LOW);
  }
  else
  {
    Serial.print("Four Relays four HV Inputs Card NOT detected!\n");
  }
  //8relay stack level 3
  if (eight_relay_card3.begin(3) )
  {
    Serial.print("Four Relays four HV Inputs Card detected\n");
    // eight_relay_card3.writeChannel(LOW);
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

  //TC Hat check.
  if (TC_Card.begin() ){
    Serial.print("TC HAT detected\n");
    TC_Check=true;
  }
  else{
    Serial.print("TC HAT NOT detected!\n");
    TC_Check=false;
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
 
  //===COOK TIMING SYSTEM===// 
  //Continusuly checks each of 6 possible ovens for timer complete.
  //Some Transmitters will have multiple Ovens attached.
  for (int j = 0; j < numOvens.Value(); j++) {
    myData.OvenID=ovenID.Value()+j;     //For systes with multiple ovens, assing myData.ovenID a unique value. 
                                        //This allows the PC data log to distingush between different ovens
                                        //connected to the same transmitter.

    //===Handle Start/Stop Button on for timer[i]===//
    //Should be compatible with up to 4 Start/Stop buttons for 4 ovens.
    if (lcd.readButtonLatch(j+2)){   //for use with LCD screen buttons
    // if (four_IO_card0.readOptoAC(i)) {        //Ideal if we can create a readButtonLatch version for relay card
      while(four_IO_card0.readOpto(j+1)){       //Wait for the button to be released. SM IO are indexed at 1
        delay(100);
      }
      delay(500);                       //Holy cow this elimiates alot of state issues. 
      Serial.print("Button Number: ");
      Serial.println(j);

     //This switch determines what to do after the Start/Stop button is pressed
      switch(status[j]) {
        case STARTUP:                   //Pre-heating
        case OVEN_READY:                //Ready
        case ACKNOWLEDGED:              //Acknowledged, Ready, or Pre-heating  
          timer[j].startTimer(cookTime.Value()*1000); //Start the timer.  cookTime is units of seconds.
          status[j] = CYCLE_ACTIVE;
          Serial.print("Timer Started for: ");
          Serial.println(cookTime.Value());
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
      for(int i = 0; i < numTCperOven.Value(); i++){
        if (TC_Check && !debug){
          myData.Temps[i] = TC_Card.readTemp(i+1);   //TC_Card.readTemp wants 1 thru 8
          //Tempreture checking
            if (myData.Temps[i] < temperatureSetpoint.Value())
            status[j] = STARTUP;
          else
            status[j] = OVEN_READY;
        }
        else
          myData.Temps[i] = random(220,250);
      }

      //Asign unused myData.Temps to zero
      for(int i = numTCperOven.Value(); i < numTCperOven.getMaxVal(); i++){
        myData.Temps[i] = 0;
      }

      //Build the myData transmission packet
      myData.Status = status[j];
      // Send message via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      if (result == ESP_OK)
        Serial.println("Sent with success");
      else
        Serial.println("Error sending the data");
      dataIntervalTimer.startTimer(dataIntervalTime.Value()*1000);   //Restart the timer

    
      SerialSendData(myData);           //Send data over USB Serial Port.  Sends a packet for ea. oven
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
  
  displayMenuOption();              // Display the current menu option on the LCD screen. Updates countdown timer. 
  updateMenuOption();               // Update the current menu option
  updateMenuValue();                // Update the menu value based on the current menu option and the rotary encoder
}
  
void SerialSendData(struct_message myData) {

  Serial.print("<");
  Serial.print(myData.OvenID);
  Serial.print(", ");
  Serial.print(myData.Count);
  Serial.print(", ");
  for(int i = 0; i < 8; i++){   
    Serial.print(myData.Temps[i]);  
    Serial.print(", ");   
  } 
  Serial.print(myData.Status); 
  Serial.print(">");
  Serial.println();
}



/*
  @brief Lookup function to determine which relay to turn on
  @param oven Oven number 0-3. Should be expanded to 8 ovens for future
  @param color enum Stacklight BUZZ(1), GRN(2), ORG(3), or RED(4).
  @param state LOW or HIGH  
*/
void manageRelays(byte oven, int color, bool state) {

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
}

/*
  @brief Determines the state of stacklights given a Status byte.  
*/
void manageStackLight(byte systemStatus, byte oven) {

  switch (systemStatus) {
    case STARTUP:
      manageRelays(oven, BUZZ, LOW);
      //delay(10);
      manageRelays(oven, GRN, LOW);
      //delay(10);
      manageRelays(oven, ORG, LOW);
      //delay(10);
      manageRelays(oven, RED, HIGH);
      break;
    case OVEN_READY:
      manageRelays(oven, BUZZ, LOW);
      //delay(10);
      manageRelays(oven, GRN, HIGH);
      //delay(10);
      manageRelays(oven, ORG, LOW);
      //delay(10);
      manageRelays(oven, RED, LOW);
      break;
    case CYCLE_ACTIVE:
      manageRelays(oven, BUZZ, LOW);
      //delay(10);
      manageRelays(oven, GRN, LOW);
      //delay(10);
      manageRelays(oven, ORG, HIGH);
      //delay(10);
      manageRelays(oven, RED, LOW);
      break;
    case TIME_COMPLETE:
      //delay(10);
      manageRelays(oven, ORG, LOW);
      //delay(10);
      manageRelays(oven, RED, LOW);
      //delay(10);
      if (StackLightOnTimers.checkTimer()==0) {  //Time comlete
        manageRelays(oven, GRN, LOW);           //Turn off the stack light
        //delay(10);                              //Seems to be necessary to turn on 2 relays at the same time.
        manageRelays(oven, BUZZ, LOW);          //Turn off the stack light
        StackLightOffTimer.startTimer(500);    //Off duration
        break;
      }

      if (StackLightOffTimer.checkTimer()==0) {   //Time copmlete
        manageRelays(oven, GRN, HIGH);            //Turn ON the stack light
        //delay(10);                                //Seems to be necessary to turn on 2 relays at the same time.
        manageRelays(oven, BUZZ, buzzerMode.Value());   //Turn ON the stack light if buzzerMode is true.
        StackLightOnTimer.startTimer(1000);    
        break;
      } 

      break;
    case ACKNOWLEDGED:
      // No action required
      manageRelays(oven, BUZZ, LOW);
      //delay(10);
      manageRelays(oven, GRN, HIGH);
      //delay(10);
      manageRelays(oven, ORG, LOW);
      //delay(10);
      manageRelays(oven, RED, LOW);
      break;
    case TEMPERATURE_DATA_SAVED:
      // No action required
      break;
    case ERROR:
      manageRelays(oven, RED, HIGH);
      //delay(10);
      manageRelays(oven, BUZZ, HIGH);
      break;
  }
}

/*
 * @brief Displays remaing Time to the LCD once every 0.250±50 seconds 
This is very poor code as millis() is called as-fast-as possible here.
 * @param oven The oven we are displaying time and status for, 0-7.
*/
void displayRemainingTime(int oven) {
  if (millis() % 250 < 50) {
      lcd.setCursor(0, 3);    //Column, Row
      lcd.print(timer[oven].remainingTime()/1000);
      lcd.print(" Sec");
      lcd.setCursor(8, 3);    //Column, Row
      lcd.print("| Status: ");
      lcd.print(status[oven]);

  } 
}
 
/*
@brief
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
        displayRemainingTime(0);
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
        lcd.print("Cook Time:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(cookTime.Value());
        lcd.print(" Minutes");
        break;
      case DATA_INTERVAL_TIME:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Data Interval Time:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(dataIntervalTime.Value());
        lcd.print(" Seconds");
        break;
      case NUM_TC_PER_OVEN:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Num T.C's per Oven:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(numTCperOven.Value());
        break;
      case NUM_OVENS:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Number of Ovens:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(numOvens.Value());
        break;
      case BUZZER_MODE:
        lcd.setCursor(0, 0);    //Column, Row
        lcd.print("Menu:");
        lcd.setCursor(0, 1);
        lcd.print("Buzzer On/ Off:");
        lcd.setCursor(0, 2);    //Column, Row
        lcd.print(buzzerMode.Value());
        break;
      case SAVE_PARAM:
        String parameters = "";
        parameters += "ovenID: " + String(ovenID.Value()) + "\n";
        myData.OvenID = ovenID.Value();     //Please fix this
        parameters += "temperatureSetpoint: " + String(temperatureSetpoint.Value()) + "\n";
        parameters += "cookTime: " + String(cookTime.Value()) + "\n";
        parameters += "dataIntervalTime: " + String(dataIntervalTime.Value()) + "\n";
        parameters += "numTCperOven: " + String(numTCperOven.Value()) + "\n";
        parameters += "numOvens: " + String(numOvens.Value()) + "\n";
        parameters += "buzzerMode: " + String(buzzerMode.Value()) + "\n";
        Serial.print(parameters); 
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
    currentMenuOption = MenuOption((currentMenuOption + 1) % 9);  //Somehow % makes sure we don't go to a non existant menu
  }
}

// Update the oven ID, temperature setpoint, cook time, or data interval time based on the current menu option and the rotary encoder
// Does not update the transmitted data packet.
void updateMenuValue() {
  lcd.resetEncoder();
  int rotaryValue = -lcd.readEncoder();
  // if (rotaryValue > 1)        //Ensure the rotary value is -1,0 or 1
  //   rotaryValue = 1;
  // else if (rotaryValue < -1)
  //   rotaryValue = -1;
  delay(10);
  
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
    case NUM_TC_PER_OVEN:
      numTCperOven.setValue(numTCperOven.Value() + rotaryValue);
      break;
    case NUM_OVENS:
      numOvens.setValue(numOvens.Value() + rotaryValue);
      break;
    case BUZZER_MODE:
      buzzerMode.setValue(buzzerMode.Value() + rotaryValue);
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
    } else if (parameterName == "numTCperOven") {
      numTCperOven.setValue(intValue);
    } else if (parameterName == "numOvens") {
      numOvens.setValue(intValue);
    } else if (parameterName == "buzzerMode") {
      buzzerMode.setValue(intValue);
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
  }


  file.close();
}
