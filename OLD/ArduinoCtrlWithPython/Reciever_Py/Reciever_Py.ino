/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

//com6

#include <esp_now.h>
#include <WiFi.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int OvenID;     //Oven Identification Number
  int BatchID;     //6-Digit identification number for each batch. 
  int Temps[8];       //Tempreture DataEach element is a seperate chamber in the oven?
  int y[8];
}struct_message;
// Create a struct_message called myData
struct_message myData;

int const log_length = 10;
//Structure to store data prior to SD card dump.  1 Struct per oven.
typedef struct struct_log {
  int id;                 //Oven and Board Number
  //int sampleCount=0;      //Sample number.  Counts non-zero rows.
  //int Tempslog_length[8];         //Each cloumn is a temp measurement from a chamber.
  int Temps[log_length][8];         //Each cloumn is a temp measurement from a chamber.
}struct_log;               //Each Row is a new sample





// Create a structure to hold the readings from each board
struct_log board1;
struct_log board2;
struct_log board3;

// Create an array with all the structures
struct_log boardsStruct[3] = {board1, board2, board3};

int sampleCount[]={0,0,0};
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes.\t", myData.OvenID, len);
  Serial.printf("SampleCount: %u \n", sampleCount[myData.OvenID-1]);
  
  // Update the structures with the new incoming data
  for(int i = 0; i < 8; i++){
      boardsStruct[myData.OvenID-1].Temps[sampleCount[myData.OvenID-1]][i] = myData.Temps[i];
    Serial.print(boardsStruct[myData.OvenID-1].Temps[sampleCount[myData.OvenID-1]][i]);
    Serial.print(", ");   
    //Serial.printf("x value: %d \n", boardsStruct[myData.OvenID-1].Temps[i]);
    //Serial.printf("y value: %d \n", boardsStruct[myData.OvenID-1].y[i]);
  }  
    Serial.println();
    sampleCount[myData.OvenID-1]++;   //Increment the number of rows in the current struct. 

    
    
    //SD Card dump
    if (sampleCount[myData.OvenID-1] >= log_length){
      Serial.println("<SD Card Dump:");
      Serial.printf("Board ID %u: %u bytes\n", myData.OvenID, len);
      
      for(int j = 0; j < sampleCount[myData.OvenID-1]; j++){
        for(int i = 0; i < 8; i++){
          Serial.print(boardsStruct[myData.OvenID-1].Temps[j][i]);
          Serial.print(", ");
        }
        Serial.print('>');
        Serial.println();
      }
      sampleCount[myData.OvenID-1] = 0;
    }
}
 
void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // Acess the variables for each board
  /*int board1X = boardsStruct[0].x;
  int board1Y = boardsStruct[0].y;
  int board2X = boardsStruct[1].x;
  int board2Y = boardsStruct[1].y;
  int board3X = boardsStruct[2].x;
  int board3Y = boardsStruct[2].y;*/

  delay(10000);  
}
