// intended for use with arduinoGUI2.py
//   this includes a function to send data to the PC

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

//=============

void setup() {
  Serial.begin(115200);
  
    // flash LEDs so we know we are alive
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
}

//=============

void loop() {
  curMillis = millis();
  getDataFromPC();
  switchLEDs();
  //~ replyToPC();
  sendToPC();
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
