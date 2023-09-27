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
*/


/*
TODO:
  - Add meta data to a single string transmisttion
      - Oven number, count, status etc.


*/


#include <esp_now.h>
#include <WiFi.h>
#include "SM_RTD.h"
#include "SM_LCDAdapter.h"
SM_LCDAdapter lcd = SM_LCDAdapter();

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0x94,0xB9,0x7E,0xF9,0x3A,0x18};

SM_RTD card(0);// RTD Data Acquisition HAT with stack level 0 (no jumpers installed)
//4 Relay 4 Input card is level 1


//int debug = 0;    //Turns off addational serial print fuctions.
int debug = 1;    //Basic Debuging.  Enables random numbers for Tempeture values if RTD hat is not detected.
bool RTD_Check;   //Check to see if the Sequent RTD hat exists.

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int OvenID[8] = {2,2,2,2,2,2,2,2}; // must be unique for each sender board.  Each Thermocouple can have a unique OvenID.
    int Count = 0;    //Counts the number of transmissions.
    //int BatchID;     //6-Digit identification number for each batch. 
    int Temps[8];   //Each element is a seperate chamber in the oven?
} struct_message;
struct_message myData;        // Create a struct_message called myData

esp_now_peer_info_t peerInfo; // Create peer interface

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

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  lcd.writeBl(20);
}

  

void loop() {
  // Set values to send
//  myData.OvenID[8] = {2,2,2,2,2,2,2,2};                      //Needs changed for each Transmitter

  //For error checking.  
  //Can implement a check on the reciever and PC to make sure no transmissions are lost.  
  myData.Count ++;
  if (debug) {
    Serial.print(myData.OvenID[0]);                      
    Serial.print(", ");
    Serial.print(myData.Count);                      
    Serial.print(", ");
  }

  for(int i = 0; i < 8; i++){
    if (RTD_Check && !debug){
      myData.Temps[i] = card.readTemp(i+1);   //card.readTemp wants 1 thru 8
    }else
      myData.Temps[i] = random(0,50);
    
    Serial.print(myData.Temps[i]);
    Serial.print(", ");
  }
  Serial.println();

  //Display tempreture data to the SM LCD screen
  DisplayTemps(myData.Temps);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(1000);
  
}




/*
Recieves an array of tempreture readings (up to 6)
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

  Temp_Disp = String("6: ") + String(x[5]);
  lcd.setCursor(10, 2);
  lcd.print(Temp_Disp);
  lcd.setCursor(19, 2);
  lcd.print("F");
  
  return;
}
