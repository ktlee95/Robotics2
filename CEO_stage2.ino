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
bool shortCut = false;
bool obstacle = false;

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
  if(!shortCut){
    unsigned int sensors[6];
    int position = reflectanceSensors.readLine(sensors);
    int error = position - 2500;
    int speedDifference = 0.15 * error + 6 * (error - lastError);
    lastError = error;
    
    if(position>2250 && position < 2750){
      m1Speed = MAX_SPEED;
      m2Speed = MAX_SPEED;
    }else if(position > 4000 || position < 1000){
      speedDifference = 0.28 * error + 6 * (error - lastError);
      m1Speed = MAX_SPEED + speedDifference;
      m2Speed = MAX_SPEED - speedDifference;
    }
    
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
    if (i%10==0)
    {
      sprintf(buf, "Detected %d:\n", blocks);
      Serial.print(buf);
      for (j=0; j<blocks; j++)
      {
        sprintf(buf, "  block %d: ", j);
        Serial.print();
        // > 50 then detect zone B
        // for bonus detect width>250
        if(pixy.blocks[j].signature == 1){
          if(pixy.blocks[j].width > 50){
            shortCut = true;
          }
        }else if(pixy.blocks[j].signature == 2){
          if(pixy.blocks[j].width > 50){
            shortCut = false;
          }
        }else if(pixy.blocks[j].signature == 3){
          if(pixy.blocks[j].width > 100){
            m1Speed = 0;
            m2Speed = 0;
            obstacle = true;
          }
        }
        
          
      }
    }
  }
  
  motors.setSpeeds(m1Speed, m2Speed);
}

