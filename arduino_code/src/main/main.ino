#include <Arduino.h>
//#include "dynamixel.h"
#include "axialMotor.h"
// We used those links to modify encoder.cpp
//https://github.com/ROBOTIS-GIT/OpenCM9.04/pull/30/files
//http://emanual.robotis.com/docs/en/parts/controller/opencr10/#layoutpin-map

// Declare global variables
const int MESSAGE_SIZE = 18;
char endOfMessageChar = '\0';
const int id3 = 221;
const int id1 = 222;
const int id2 = 223;
#define START_CALIBRATION 0
#define NO_CALIBRATION -2

//Dynamixel mot1(id1, 0.879); //28
//Dynamixel mot2(id2, 0.879*2); //40
//Dynamixel mot3(id3, 1); //20


/**
   \struct dataPack
   \brief A structure containing message data
*/
struct dataPack {
  //! Motor 1 position
  uint16_t p1;
  //! Motor 2 position
  uint16_t p2;
  //! Motor 3 position
  uint16_t p3;
  //! Motor 4 position
  uint16_t p4; // Vertical motor
  //! Motor 5 position
  uint16_t p5;
  //! Motor 6 position
  uint16_t p6;
  //! Stop indicator
  bool shouldStop;
  //! Drawer 1 state
  bool drawer1;
  //! Drawer 2 state
  bool drawer2;
  //! Drawer 3 state
  bool drawer3;
  //! Operation mode
  char mode;
  //! End of message character
  char last;
};

// Function prototypes
bool readDataToStruct(dataPack *data);
void readMessage(char *message);
void sendMessage(dataPack message);
void moveAbsolute(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5, uint16_t p6);
void moveIncremental(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5, uint16_t p6);
void setDrawerGoalState(char drawer1, char drawer2, char drawer3);
void stopMotors();
void startMotors();

void trigShouldSlowDownPin1();
void trigShouldSlowDownPin2();

//axialMotor axialMotor(53,-1,A1,A2,19,20,2,3);
axialMotor test; //classe test

// Arduino functions
void setup() {
  Serial.begin(9600); // set the baud rate, must be the same for both machines
  //while (!Serial);
//  mot1.init();
//  mot2.init();
//  mot3.init();
  //dataPack outgoingMessage{(byte)'s',(int32_t)(mot1.getPosition()), (int32_t)(mot2.getPosition()), (int32_t)(mot3.getPosition()), 0, 0, 0, (byte)'\0'};
  //sendMessage(outgoingMessage);
  pinMode(test.getProximitySensorPin(1), INPUT_PULLUP); //Set input as a pull-up for proximity sensor
  pinMode(test.getProximitySensorPin(2), INPUT_PULLUP); //Set input as a pull-up for proximity sensor
  attachInterrupt(digitalPinToInterrupt(test.getProximitySensorPin(1)), trigShouldSlowDownPin1, FALLING);
  attachInterrupt(digitalPinToInterrupt(test.getProximitySensorPin(2)), trigShouldSlowDownPin2, FALLING);
  pinMode(test.getMotorPin(1),OUTPUT);
  pinMode(test.getMotorPin(2),OUTPUT);
  pinMode(test.getDrivePin(),OUTPUT);

  pinMode(38,OUTPUT); //power for one of the sensor
  digitalWrite(38,HIGH); //power for one of the sensor
  test.setEnableDrive(true);
  test.modifyCalibrationCase(NO_CALIBRATION); //START_CALIBRATION NO_CALIBRATION
  test.setMotorState(-1);
}
bool buttonCalibration = false;
int requiredPosition = 1250; //from 0 (TOP) to 4095 (BOT)
bool slowItTOP = false;
bool slowItBOT = false;
uint16_t goalPositionVerticalAxis = 0;

void loop() {
  //test.runIt(&slowItTOP,&slowItBOT,requiredPosition,&buttonCalibration);
  //Serial.print("RETOUR DE GETPOSITION: ");
  //Serial.println(test.getPosition(test.getCalibrationCase()));
  //test.getPosition(encPosition,test.getCalibrationCase());
  test.runIt(&slowItTOP,&slowItBOT,goalPositionVerticalAxis,&buttonCalibration);//data.buttonCalibration
  if (Serial.available() >= MESSAGE_SIZE) // Only parse message when the full message has been received.
  {
    
    // Read data
    dataPack data;
    
    if (readDataToStruct(&data))
    {
      // Debug
      //Serial.println(data.p1);
      //Serial.println(data.p2);
      //Serial.println(data.p3);
      //Serial.println(data.p4);
      //Serial.println(data.p5);
      //Serial.println(data.p6);
      //Serial.println(data.end);
      //byte* serializedMessage = (byte*)&data, sizeof(data);
      //Serial.println(serializedMessage);
      if(data.shouldStop == false)
      {
        if(data.mode == 'a')
        {
          goalPositionVerticalAxis = data.p4;
          //moveAbsolute(data.p1, data.p2, data.p3, data.p4, data.p5, data.p6);
          setDrawerGoalState(data.drawer1, data.drawer2, data.drawer3);
        }
        else if(data.mode == 'i')
        {
          moveIncremental(data.p1, data.p2, data.p3, data.p4, data.p5, data.p6);
          setDrawerGoalState(data.drawer1, data.drawer2, data.drawer3);
        }
        else if(data.mode == 's')
        {
          // Do nothing apart from sending message
        }
        else if(data.mode == 'c')
        {
          buttonCalibration = true;
        }
      }
      else
      {
        stopMotors();
        startMotors();
      }
                               
      dataPack outgoingMessage{1,
                               2,
                               3,
                               test.getPosition(test.getCalibrationCase()), //test.getPosition(test.getCalibrationCase())
                               5,
                               6,
                               (bool) data.shouldStop , //data.shouldStop
                               (bool) data.drawer1, //data.drawer1
                               (bool) data.drawer2, //data.drawer2
                               (bool) data.drawer3, //data.drawer3
                               (char)data.mode,
                               (char)'\0'};
      sendMessage(outgoingMessage);
    }
    else
    {
      //Serial.println("Failed to parse message");
    }
  }
}

// Functions

/** \brief Iterates through message one character at a time until the end character is found and returns a char array of the message.
    \param message : empty character array
    \return message : character array containing message
*/
void readMessage(char *message)
{
  int count = 0;
  char c;

  do
  {
    if (Serial.available())
    {
      c = Serial.read();
      message[count] = c;
      count++;
    }
  } while (c != '\0');
}


/** \brief Iterates through message one byte at a time casts it to a dataPack struct.
    \param data : empty dataPack object the message will be written to
    \return data : dataPack object containging the unpacked message data
*/
bool readDataToStruct(dataPack *data)
{
  int i = 0;
  byte buf[MESSAGE_SIZE];
  while (Serial.available() && i < MESSAGE_SIZE)
  {
    buf[i] = Serial.read();
    i++;
  }
  memcpy(data, buf, sizeof(*data));
  if (data->last != endOfMessageChar) // if the last character is not the end-of-message character, message is corrupted
    return false;

  return true;
}


/** \brief Sends an encoded message over serial
    \param message : empty dataPack object the message will be written to
*/
void sendMessage(dataPack message)
{
  Serial.write((byte*)&message, sizeof(message));
}


/** \brief move motors to an absolute position
    \param p1, ..., p6 : position for each motor
*/
void moveAbsolute(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5, uint16_t p6)
{
//    mot1.moveMotor(p1);
//    mot2.moveMotor(p2);
//    mot3.moveMotor(p3);
}

/** \brief move motors to an incremental position
    \param p1, ..., p6 : position for each motor
*/
void moveIncremental(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4, uint16_t p5, uint16_t p6)
{
  //TODO
}

void setDrawerGoalState(char drawer1, char drawer2, char drawer3)
{
  //TODO
}

void stopMotors()
{
//  mot1.torque(false);
//  mot2.torque(false);
//  mot3.torque(false);
}

void startMotors()
{
//  mot1.torque(true);
//  mot2.torque(true);
//  mot3.torque(true);
}

void trigShouldSlowDownPin1()
{
  //Serial.println("TOP MAX");
    slowItTOP = true;
    if(test.shouldSlowDown(slowItTOP,slowItBOT) == true && test.getCalibrationCase() != 0)
    {
      //Serial.println("STOP MOTOR");
      test.setMotorState(-1);
    }
}

void trigShouldSlowDownPin2()
{
  //Serial.println("BOTTOM MAX");
  slowItBOT = true;

  if(test.shouldSlowDown(slowItTOP,slowItBOT) == true)
  {
    //Serial.println("STOP MOTOR");
    test.setMotorState(-1);
  }
}
