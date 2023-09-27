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
byte ledPin[numLEDs] = {2, 13};
byte ledStatus[numLEDs] = {0, 0};

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
  int OvenID[8];     //Oven Identification Number
  int Count = 0;    //Counts the number of transmissions.
  //int BatchID;     //6-Digit identification number for each batch. 
  int Temps[8];       //Tempreture Data. Each element is a seperate chamber in the oven?
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

  //Send data over serial ports.
  Serial.print("<");
  Serial.print(myData.OvenID);
  Serial.print(", ");
  Serial.print(myData.Count);
  Serial.print(", ");
  for(int i = 0; i < 8; i++){
  // Update the structures with the new incoming data.  For Synchronizing Delay  card storage
    //boardsStruct[myData.OvenID-1].Temps[sampleCount[myData.OvenID-1]][i] = myData.Temps[i];
    //Serial.print(boardsStruct[myData.OvenID-1].Temps[sampleCount[myData.OvenID-1]][i]);
    
    Serial.print(myData.Temps[i]);  
    Serial.print(", ");   

  }  
    Serial.print(">");
    Serial.println();
    //sampleCount[myData.OvenID-1]++;   //Increment the number of rows in the current struct. 

    
    
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
    // flash LEDs so we know we are alive

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info

    
  for (byte n = 0; n < numLEDs; n++) {
     pinMode(ledPin[n], OUTPUT);
     digitalWrite(ledPin[n], HIGH);
  }
  delay(500); // delay() is OK in setup as it only happens once
  
  for (byte n = 0; n < numLEDs; n++) {
     digitalWrite(ledPin[n], LOW);
  }
    
    // tell the PC we are ready
  Serial.println("<Arduino is ready>");
  esp_now_register_recv_cb(OnDataRecv);
}

//=============

void loop() {
  curMillis = millis();
  getDataFromPC();
  //switchLEDs();
  //~ replyToPC();
  //sendToPC();
  delay(1000);
}

//=============

void getDataFromPC() {

    // receive data from PC and save it into inputBuffer
    
  if(Serial.available() > 0) {

    char x = Serial.read();

      // the order of these IF clauses is significant
      
    if (x == endMarker) {
      readInProgress = false; 
      newDataFromPC = true;
      inputBuffer[bytesRecvd] = 0;
      parseData();
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

//=============
 
void parseData() {

    // split the data into its parts
    // assumes the data will be received as (eg) 0,1,35
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(inputBuffer,","); // get the first part
  ledStatus[0] = atoi(strtokIndx); //  convert to an integer
  
  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  ledStatus[1] = atoi(strtokIndx);
  
 

}

//=============

void replyToPC() {

  if (newDataFromPC) {
    newDataFromPC = false;
    Serial.print("<LedA ");
    Serial.print(ledStatus[0]);
    Serial.print(" LedB ");
    Serial.print(ledStatus[1]);
    Serial.print(" SrvPos ");
    Serial.print(50);
    Serial.print(" Time ");
    Serial.print(curMillis >> 9); // divide by 512 is approx = half-seconds
    Serial.println(">");
  }
}

//=============

void switchLEDs() {

  for (byte n = 0; n < numLEDs; n++) {
    digitalWrite( ledPin[n], ledStatus[n]);
  }
}


//=============

void sendToPC() {
	if (curMillis - prevReplyToPCmillis >= replyToPCinterval) {
		prevReplyToPCmillis += replyToPCinterval;
		int valForPC = curMillis >> 9; // approx half seconds
		Serial.print('<');
    Serial.print("0,1,2,3,4,5,6,7");
		//Serial.print(valForPC);
		Serial.print('>');
	}
	
}
