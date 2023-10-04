#include "Arduino.h"
#include "Timer.h"

//Constructor Deffinition.  executes whenever an object is created
Timer::Timer() {
  _timerStartTime = millis();
}

void Timer::startTimer(int timerInterval) {
  _timerInterval = timerInterval;
  _timerStartTime = millis();
  Serial.print("Starting timer for: ");
  Serial.print(_timerInterval);
  Serial.println(" mS");
}


int Timer::checkTimer() {
  unsigned long currentTime = millis();
  if ((_timerInterval != 0)&&(currentTime - _timerStartTime >= _timerInterval)){
    Serial.print(_timerInterval);
    Serial.println(" mS has elapsed!");
    _timerInterval = 0UL;  //Unused timer
    return 1;   //Time us up
  }
  return 0;
}
