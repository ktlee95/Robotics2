//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//
// This sketch is a good place to start if you're just getting started with 
// Pixy and Arduino.  This program simply prints the detected object blocks 
// (including color codes) through the serial console.  It uses the Arduino's 
// ICSP port.  For more information go here:
//
// http://cmucam.org/projects/cmucam5/wiki/Hooking_up_Pixy_to_a_Microcontroller_(like_an_Arduino)
//
// It prints the detected blocks once per second because printing all of the 
// blocks for all 50 frames per second would overwhelm the Arduino's serial port.
//
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
#include <ZumoMotors.h>
#include <ZumoBuzzer.h>
#include <Pushbutton.h>
#include <SPI.h>  
#include <Pixy.h>

// This is the main Pixy object 
Pixy pixy;

//motors
ZumoBuzzer buzzer;
ZumoReflectanceSensorArray reflectanceSensors;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON);
int lastError = 0;
int m1Speed = 0;
int m2Speed = 0;
const int MAX_SPEED = 400;

//bool shortCut = false;
int stage = 1;
const int objectWidth = 100;
const int objectWidth2 = 70;
const int minObjectWidth2 = 15;
void setup()
{
  Serial.begin(9600);
  Serial.print("Starting...\n");
  motors.setSpeeds(0, 0);
  




  // Play a little welcome song
  buzzer.play(">g32>>c32");

  // Initialize the reflectance sensors module
  reflectanceSensors.init();

  // Wait for the user button to be pressed and released
//  button.waitForButton();

  // Turn on LED to indicate we are in calibration mode
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Wait 1 second and then begin automatic sensor calibration
  // by rotating in place to sweep the sensors over the line
  delay(1000);
  int i;
  for(i = 0; i < 80; i++)
  {
    if ((i > 10 && i <= 30) || (i > 50 && i <= 70))
      motors.setSpeeds(-200, 200);
    else
      motors.setSpeeds(200, -200);
    reflectanceSensors.calibrate();

    // Since our counter runs to 80, the total delay will be
    // 80*20 = 1600 ms.
    delay(20);
  }
  motors.setSpeeds(0,0);

  // Turn off LED to indicate we are through with calibration
  digitalWrite(13, LOW);
  pixy.init();
  buzzer.play(">g32>>c32");
  
  // Wait for the user button to be pressed and released
//  button.waitForButton();

  
  // Play music and wait for it to finish before we start driving.
  delay(3000);
  buzzer.play("L16 cdegreg4");
//  while(buzzer.isPlaying());
  

  
}

void loop()
{
  if(stage == 1){
    unsigned int sensors[6];

    // Get the position of the line.  Note that we *must* provide the "sensors"
    // argument to readLine() here, even though we are not interested in the
    // individual sensor readings
    int position = reflectanceSensors.readLine(sensors);
  
    // Our "error" is how far we are away from the center of the line, which
    // corresponds to position 2500.
    int error = position - 2500;
    
    // Get motor speed difference using proportional and derivative PID terms
    // (the integral term is generally not very useful for line following).
    // Here we are using a proportional constant of 1/4 and a derivative
    // constant of 6, which should work decently for many Zumo motor choices.
    // You probably want to use trial and error to tune these constants for
    // your particular Zumo and line course.
    int speedDifference = 0.15 * error + 40 * (error - lastError);
    
    lastError = error;
    m1Speed = MAX_SPEED + speedDifference;
    m2Speed = MAX_SPEED - speedDifference;
    
    // Here we constrain our motor speeds to be between 0 and MAX_SPEED.
    // Generally speaking, one motor will always be turning at MAX_SPEED
    // and the other will be at MAX_SPEED-|speedDifference| if that is positive,
    // else it will be stationary.  For some applications, you might want to
    // allow the motor speed to go negative so that it can spin in reverse.
    if (m1Speed < 0)
      m1Speed = 0;
    if (m2Speed < 0)
      m2Speed = 0;
    if (m1Speed > MAX_SPEED)
      m1Speed = MAX_SPEED;
    if (m2Speed > MAX_SPEED)
      m2Speed = MAX_SPEED;
  }
  
  
  static int i = 0;
  int j;
  uint16_t blocks;
  char buf[32]; 
  // grab blocks!
  blocks = pixy.getBlocks();
  
  // If there are detect blocks, print them!
  if (blocks)
  {
    i++;
    
    // do this (print) every 50 frames because printing every
    // frame would bog down the Arduino
    if (i%1==0)
    {
      sprintf(buf, "Detected %d:\n", blocks);
      Serial.print(buf);
      for (j=0; j<blocks; j++)
      {
        sprintf(buf, "  block %d: ", j);
//        Serial.print();
        // > 50 then detect zone B
        // for bonus detect width>250
        if((pixy.blocks[j].signature == 1) && (stage == 1)){
          if(pixy.blocks[j].width > objectWidth){
            stage = 2;
          }
        }else if((pixy.blocks[j].signature == 2) && (stage == 2 &&(pixy.blocks[j].width > minObjectWidth2))){
          if(pixy.blocks[j].x > (319/3) * 2){
            //turn right
            m1Speed = 150;
            m2Speed = -150;
            Serial.println("I detect it on my right");
          }
          else if(pixy.blocks[j].x < (319/3) * 1){
            
            //turn left
            m1Speed = -150;
            m2Speed = 150;
            Serial.println("I detect it on my left");
          }
          else{
            m1Speed = MAX_SPEED;
            m2Speed = MAX_SPEED;
            Serial.println("I detect it on my center");
            if(pixy.blocks[j].width > objectWidth2){
              stage = 1;
            }
          }
        }else if(stage == 3){
          m1Speed = 0;
          m2Speed = 0;
        }
      }
    }
  }else{
    if(stage == 2){
      m1Speed = 150;
      m2Speed = -150;
    }
  }
  if(stage == 3){
    m1Speed = 0;
    m2Speed = 0;
  }
  
  motors.setSpeeds(m1Speed, m2Speed);
}

