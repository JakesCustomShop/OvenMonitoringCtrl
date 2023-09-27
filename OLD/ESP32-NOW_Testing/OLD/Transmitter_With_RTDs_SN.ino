/*********
  Rui Santos ESP_NOW Example
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/

*********/

/*!
    Temperature reading usage example for Sequent Microsystems RTD Data Acquisition HAT
    -->https://github.com/SequentMicrosystems/Sequent-RTD-Library
    Requirments: Any arduino card with I2C, RTD Data Acquisition HAT from Sequent Microsystems
    The example is more usefull if you connect one RTD sensors to all channels.

    Note: Must use 3-Wire RTD or jumper on RTD DAQ. inputs
*/

#include <esp_now.h>
#include <WiFi.h>
#include "SM_RTD.h"

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = {0x94,0xB9,0x7E,0xF9,0x3A,0x18};

SM_RTD card(0);// RTD Data Acquisition HAT with stack level 0 (no jumpers installed)

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int OvenID;     //must be unique for each sender board
    int BatchID;     //6-Digit identification number for each batch. 
    int x[8];   //Each element is a seperate chamber in the oven?
    int y[8];
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  //Turn on the LED on the ESP-PI.  Manual control only.
  //digitalWrite(SM_ESP32PI_LED, HIGH);//builtin function with library definition SM_ESP32PI_LED

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
  // Set values to send
  myData.OvenID = 2;                      //Needs changed for each Transmitter

  
  for(int i = 0; i < 8; i++){
    myData.x[i] = card.readTemp(i+1);   //card.readTemp wants 1 thru 8
    myData.y[i] = random(0,50);         //Placeholder for future data.  Currently unused.
    Serial.print(myData.x[i]);
    Serial.print(", ");
  }
  Serial.println();

 
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
