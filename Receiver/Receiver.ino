/*
  Jake Lehotsky
  Jake's Custom Shop, LLC
  August, 2023

  Recieves serial data from Wireless Transmistters.
  Formats the data
  Sends data packets to a PC via USB. 
  Any serial.print data wraped with < and > will be recieved by the PC

  Notes:
   - Each transmitter must contain the Recievers MAC address
   - For use with Transmitter_With_RTDs.ino
   - Intended for use with arduinoGUI2.py
   - A lot of extra stuff in here that should be deleted. 
*/



#include <esp_now.h>
#include <WiFi.h>

const byte numLEDs = 2;

const byte buffSize = 40;
char inputBuffer[buffSize];
const char startMarker = '<';
const char endMarker = '>';
byte bytesRecvd = 0;
boolean readInProgress = false;
boolean newDataFromPC = false;

char messageFromPC[buffSize] = {0};

unsigned long curMillis;

unsigned long prevReplyToPCmillis = 0;
unsigned long replyToPCinterval = 3500;



// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int OvenID;     //Oven Identification Number
  int Count = 0;    //Counts the number of transmissions.
  int Temps[8];       //Tempreture Data. Each element is a seperate chamber in the oven?
  byte Status;
  //int BatchID;     //6-Digit identification number for each batch. 
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
  
  //Serial.print("<Packet received from: >");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  //Serial.printf("<Board ID %u: %u bytes.\t >", myData.OvenID, len);
  //Serial.printf("<SampleCount: %u \n >", sampleCount[myData.OvenID-1]);


  //===SEND DATA OVER SERIAL PORTS===//
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
    sampleCount[myData.OvenID-1]++;   //Increment the number of rows in the current struct. 

    
    
    //SD Card dump
    //Compiles reading from all RTDs on one oven into a single 2D structure.  Represents a single cooking cycle.
    /*
    if (sampleCount[myData.OvenID-1] >= log_length){
      Serial.println("SD Card Dump:");
      Serial.printf("Board ID %u: %u bytes\n", myData.OvenID, len);
      for(int j = 0; j < sampleCount[myData.OvenID-1]; j++){
        for(int i = 0; i < 8; i++){
          Serial.print(boardsStruct[myData.OvenID-1].Temps[j][i]);
          Serial.print(", ");
        }
        Serial.println();
      }
      sampleCount[myData.OvenID-1] = 0;
    }*/
}

//=============

void setup() {
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
     
      
  // tell the PC we are ready
  Serial.println("<Arduino is ready>");
  esp_now_register_recv_cb(OnDataRecv);
}

//=============

void loop() {
}
