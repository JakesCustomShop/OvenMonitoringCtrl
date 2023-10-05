#include "Arduino.h"
#include "Timer.h"

//Constructor Deffinition.  executes whenever an object is created
Timer::Timer() {
  _timerStartTime = millis();
}

void Timer::startTimer(int timerInterval) {
  _timerInterval = timerInterval;
  _timerStartTime = millis();
  //Serial.print("Starting timer for: ");
  //Serial.print(_timerInterval);
  //Serial.println(" mS");
}


int Timer::checkTimer() {
  unsigned long currentTime = millis();
  if ((_timerInterval != 0)&&(currentTime - _timerStartTime >= _timerInterval)){
//    Serial.print(remainingTime);
//    Serial.println(" mS remaining");
    _timerInterval = 0UL;  //Unused timer
    return 0;   //Timer Complete
  }
  return 1;   //Timer Active
}


int Timer::remainingTime() {
  unsigned long currentTime = millis();
  if ((currentTime - _timerStartTime <= _timerInterval)){
    return _timerInterval - (currentTime - _timerStartTime);
  }
  return 0;
}
