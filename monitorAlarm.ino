#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"


#define RXD2 16
#define TXD2  17

DFRobotDFPlayerMini myDFPlayer;

const int statusLedPin = 18;
const int callLedPin = 15;
const int callPin = 25;
const int dismissPin = 26;
const int usbPresentPin = 35;
const int batteryLedPin = 22;
const int sirenPin = 21;

int emergencyTime1 = 10000;
int emergencyTime2 = 20000;
int emergencyStageTime;
const int blinkDelay = 300;
const int volume = 30;

boolean batteryBackup = false;
boolean called = false;
boolean emergency = false;
double timeOfLastCall = 0;
double timeOfLastBlink = 0;

void setup() {
  pinMode(statusLedPin, OUTPUT);
  pinMode(callLedPin, OUTPUT);
  pinMode(batteryLedPin, OUTPUT);
  pinMode(callPin, INPUT_PULLUP);
  pinMode(dismissPin, INPUT_PULLUP);
  pinMode(usbPresentPin, INPUT);
  pinMode(sirenPin, OUTPUT);

  digitalWrite(statusLedPin, HIGH);
  digitalWrite(callLedPin, HIGH);
  digitalWrite(batteryLedPin, HIGH);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  

  Serial.println();

  if (!myDFPlayer.begin(Serial2)) { 
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    errorMode(2, "Unable to connect to mp3 player");
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  //Enter delay programming mode by pressing dismiss button on startup
  if(digitalRead(dismissPin) == LOW)
  {
    int buttonPresses = 1;
    boolean exit = false;

    beep(1);

    //Wait until user has released button
    while(true)
    {
      if(digitalRead(dismissPin) == HIGH)
        break;
    }

    //Start timer. After 3000ms of not pressing the button, exit the programming mode
    double timeElapsed = millis();

    while(!exit)
    {
      if(digitalRead(dismissPin) == LOW)
      {
        buttonPresses++;
        timeElapsed = millis();
        switch(buttonPresses)
        {
          case 1:
            emergencyTime1 = 10000;
            beep(1);
            break;
          case 2:
            emergencyTime1 = 20000;
            beep(2);
            break;
          case 3:
            emergencyTime1 = 30000;
            beep(3);
            break;
          default:
            buttonPresses = 1;
            emergencyTime1 = 10000;
            beep(1);
            break;
        }
        while(true)
        {
          if(digitalRead(dismissPin) == HIGH)
            break;
        }
      }
      
      if((millis() - timeElapsed) > 3000)
      {
        exit = true;
      }
    }
    beep(buttonPresses); //Confirm the emergency time

  }

  
  
  myDFPlayer.volume(volume);  //Set volume value. From 0 to 30
  myDFPlayer.play(1);  //Play the first mp3 to indicate speaker is working
  digitalWrite(callLedPin, LOW);

}

void loop()
{
  static unsigned long timer = millis();
  if(digitalRead(usbPresentPin) == LOW && !batteryBackup)
  {
    myDFPlayer.play(1);
    digitalWrite(batteryLedPin, HIGH);
    batteryBackup = true;
  }

  if(batteryBackup)
  {
    if((millis() - timeOfLastBlink) > blinkDelay)
    {
      digitalWrite(batteryLedPin, !digitalRead(batteryLedPin));
      timeOfLastBlink = millis();
    }
    if(digitalRead(usbPresentPin) == HIGH)
    {
      batteryBackup = false;
    }
  }

  else if(digitalRead(usbPresentPin) == HIGH)
  {
    digitalWrite(batteryLedPin, LOW);
    digitalWrite(statusLedPin, HIGH);
    batteryBackup = false;
  }

  if(digitalRead(callPin) == LOW && !called)
  {
    digitalWrite(callLedPin, HIGH);
    myDFPlayer.play(1);
    Serial.println("Called");
    called = true;
    timeOfLastCall = millis();
  }

  else if(digitalRead(callPin) == LOW && called && !emergency)
  {
    if((millis() - timeOfLastCall) > 3000)
    {
      myDFPlayer.loop(2);
      emergency = true;
      emergencyStageTime = millis();
      delay(10);
    }
  }

  //Emergency stage 1 - plays emergency tone on speaker
  if(called && (millis() - timeOfLastCall)>emergencyTime1 && !emergency)
  {
    myDFPlayer.loop(2);
    emergency = true;
    emergencyStageTime = millis();
  }

  //Emergency stage 2 - activates sirens 
  if(called && emergency && (millis() - emergencyStageTime)>emergencyTime2)
  {
    digitalWrite(sirenPin, HIGH);
  }


  if(digitalRead(dismissPin) == LOW && called)
  {
    myDFPlayer.pause();
    myDFPlayer.disableLoop();
    called = false;
    emergency = false;
    digitalWrite(callLedPin, LOW);
    digitalWrite(sirenPin, LOW);
  }
}

void errorMode(int errMode, String errMsg)
{
  Serial.println(errMsg);
  while (true)
  {
    digitalWrite(statusLedPin, LOW);
    delay(300);
    for(int i=0; i<errMode; i++)
    {
      digitalWrite(statusLedPin, HIGH);
      delay(200);
      digitalWrite(statusLedPin, LOW);
      delay(200);
    }
  }
  
  
}

void beep(int numTimes)
{
  for(int i=0; i<numTimes; i++)
  {
    digitalWrite(sirenPin, HIGH);
    delay(5);
    digitalWrite(sirenPin, LOW);
    delay(100);
  }
}
