//mode 1 code
#include <Servo.h>
#include <EEPROM.h>
#include <uSTimer2.h>
#include <CharliePlexM.h>
#include <Wire.h>
#include <I2CEncoder.h>

// Uncomment keywords to enable debugging output

//#define DEBUG_MODE_DISPLAY
//#define DEBUG_MOTORS
//#define DEBUG_LINE_TRACKERS
//#define DEBUG_ENCODERS
//#define DEBUG_ULTRASONIC_FRONT
//#define DEBUG_ULTRASONIC_RIGHT
//#define DEBUG_ULTRASONIC_LEFT
//#define DEBUG_LINE_TRACKER_CALIBRATION
//#define DEBUG_MOTOR_CALIBRATION

boolean bt_Motors_Enabled = true;

//port pin constants
const int ci_Ultrasonic_Data_Front = 2;   //input plug
const int ci_Ultrasonic_Ping_Front = 3;   //output plug
//right side ultrasonic
const int ci_Ultrasonic_Data_Right = 4;   //input plug
const int ci_Ultrasonic_Ping_Right = 5;   //output plug
//left side ultrasonic
const int ci_Ultrasonic_Data_Left = 6;   //input plug
const int ci_Ultrasonic_Ping_Left = 7;   //output plug

const int ci_Charlieplex_LED1 = 1;//4
const int ci_Charlieplex_LED2 = 1;//5
const int ci_Charlieplex_LED3 = 1;//6
const int ci_Charlieplex_LED4 = 1;//7

const int ci_Mode_Button = 0;
const int ci_Right_Motor = 8;
const int ci_Left_Motor = 9;
const int ci_Motor_Enable_Switch = 12;
const int ci_Right_Line_Tracker = A0;
const int ci_Middle_Line_Tracker = A1;
const int ci_Left_Line_Tracker = A2;
const int ci_I2C_SDA = A4;         // I2C data = white
const int ci_I2C_SCL = A5;         // I2C clock = yellow


// Charlieplexing LED assignments
/*const int ci_Heartbeat_LED = 1;
  const int ci_Indicator_LED = 4;
  const int ci_Right_Line_Tracker_LED = 6;
  const int ci_Middle_Line_Tracker_LED = 9;
  const int ci_Left_Line_Tracker_LED = 12;
  const int ci_ultrasonic_LED = 8; // pin LED for ultrasonic sensor*/

const int ci_servo_turntable = 13;
const int ci_servo_pivot = 12;
const int ci_servo_arm = 11;
const int ci_servo_magnet = 10;

//constants

// EEPROM addresses
const int ci_Left_Line_Tracker_Dark_Address_L = 0;
const int ci_Left_Line_Tracker_Dark_Address_H = 1;
const int ci_Left_Line_Tracker_Light_Address_L = 2;
const int ci_Left_Line_Tracker_Light_Address_H = 3;
const int ci_Middle_Line_Tracker_Dark_Address_L = 4;
const int ci_Middle_Line_Tracker_Dark_Address_H = 5;
const int ci_Middle_Line_Tracker_Light_Address_L = 6;
const int ci_Middle_Line_Tracker_Light_Address_H = 7;
const int ci_Right_Line_Tracker_Dark_Address_L = 8;
const int ci_Right_Line_Tracker_Dark_Address_H = 9;
const int ci_Right_Line_Tracker_Light_Address_L = 10;
const int ci_Right_Line_Tracker_Light_Address_H = 11;
const int ci_Left_Motor_Offset_Address_L = 12;
const int ci_Left_Motor_Offset_Address_H = 13;
const int ci_Right_Motor_Offset_Address_L = 14;
const int ci_Right_Motor_Offset_Address_H = 15;

const int ci_Left_Motor_Stop = 1500;        // 200 for brake mode; 1500 for stop
const int ci_Right_Motor_Stop = 1500;
const int ci_Display_Time = 500;
const int ci_Line_Tracker_Calibration_Interval = 100;
const int ci_Line_Tracker_Cal_Measures = 20;
const int ci_Line_Tracker_Tolerance = 50;   // May need to adjust this
const int ci_Motor_Calibration_Cycles = 3;
const int ci_Motor_Calibration_Time = 5000;

//variables
byte b_LowByte;
byte b_HighByte;
unsigned long ul_Echo_Time_Front;
unsigned long ul_Echo_Time_Right;
unsigned long ul_Echo_Time_Left;

unsigned int ui_Left_Line_Tracker_Data;
unsigned int ui_Middle_Line_Tracker_Data;
unsigned int ui_Right_Line_Tracker_Data;
unsigned int ui_Motors_Speed = 1900;        // Default run speed
unsigned int ui_Left_Motor_Speed;
unsigned int ui_Right_Motor_Speed;
long l_Left_Motor_Position;
long l_Right_Motor_Position;

unsigned long ul_3_Second_timer = 0;
unsigned long ul_Display_Time;
unsigned long ul_Calibration_Time;
unsigned long ui_Left_Motor_Offset;
unsigned long ui_Right_Motor_Offset;

unsigned int ui_Cal_Count;
unsigned int ui_Cal_Cycle;
unsigned int ui_Left_Line_Tracker_Dark;
unsigned int ui_Left_Line_Tracker_Light;
unsigned int ui_Middle_Line_Tracker_Dark;
unsigned int ui_Middle_Line_Tracker_Light;
unsigned int ui_Right_Line_Tracker_Dark;
unsigned int ui_Right_Line_Tracker_Light;
unsigned int ui_Line_Tracker_Tolerance;

unsigned int  ui_Robot_State_Index = 0;
//0123456789ABCDEF
unsigned int  ui_Mode_Indicator[6] = {
  0x00,    //B0000000000000000,  //Stop
  0x00FF,  //B0000000011111111,  //Run
  0x0F0F,  //B0000111100001111,  //Calibrate line tracker light level
  0x3333,  //B0011001100110011,  //Calibrate line tracker dark level
  0xAAAA,  //B1010101010101010,  //Calibrate motors
  0xFFFF   //B1111111111111111   //Unused
};

unsigned int  ui_Mode_Indicator_Index = 0;

//display Bits 0,1,2,3, 4, 5, 6,  7,  8,  9,  10,  11,  12,  13,   14,   15
int  iArray[16] = {
  1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 65536
};
int  iArrayIndex = 0;

boolean bt_Heartbeat = true;
boolean bt_3_S_Time_Up = false;
boolean bt_Do_Once = false;
boolean bt_Cal_Initialized = false;

Servo servo_RightMotor;
Servo servo_LeftMotor;

Servo servo_turntable, servo_pivot, servo_arm, servo_magnet;

I2CEncoder encoder_RightMotor;
I2CEncoder encoder_LeftMotor;

/*****************************************************************SETUP************************************************************/
void setup() {
  Wire.begin();
  Serial.begin(9600);

  //setup ultrasonic
  pinMode(ci_Ultrasonic_Ping_Front, OUTPUT);
  pinMode(ci_Ultrasonic_Data_Front, INPUT);
  pinMode(ci_Ultrasonic_Ping_Right, OUTPUT);
  pinMode(ci_Ultrasonic_Data_Right, INPUT);
  pinMode(ci_Ultrasonic_Ping_Left, OUTPUT);
  pinMode(ci_Ultrasonic_Data_Left, INPUT);

  CharliePlexM::setBtn(ci_Charlieplex_LED1, ci_Charlieplex_LED2,
                       ci_Charlieplex_LED3, ci_Charlieplex_LED4, ci_Mode_Button);

  // set up drive motors
  pinMode(ci_Right_Motor, OUTPUT);
  servo_RightMotor.attach(ci_Right_Motor);
  pinMode(ci_Left_Motor, OUTPUT);
  servo_LeftMotor.attach(ci_Left_Motor);

  // set up motor enable switch
  pinMode(ci_Motor_Enable_Switch, INPUT);

  // set up encoders. Must be initialized in order that they are chained together,
  // starting with the encoder directly connected to the Arduino. See I2CEncoder docs
  // for more information
  encoder_LeftMotor.init(1.0 / 3.0 * MOTOR_393_SPEED_ROTATIONS, MOTOR_393_TIME_DELTA);
  encoder_LeftMotor.setReversed(false);  // adjust for positive count when moving forward
  encoder_RightMotor.init(1.0 / 3.0 * MOTOR_393_SPEED_ROTATIONS, MOTOR_393_TIME_DELTA);
  encoder_RightMotor.setReversed(true);  // adjust for positive count when moving forward

  // set up line tracking sensors
  pinMode(ci_Right_Line_Tracker, INPUT);
  pinMode(ci_Middle_Line_Tracker, INPUT);
  pinMode(ci_Left_Line_Tracker, INPUT);
  ui_Line_Tracker_Tolerance = ci_Line_Tracker_Tolerance;

  // read saved values from EEPROM
  b_LowByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Left_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Left_Line_Tracker_Light_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Left_Line_Tracker_Light = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Middle_Line_Tracker_Dark_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Middle_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Middle_Line_Tracker_Light_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Middle_Line_Tracker_Light = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Right_Line_Tracker_Dark_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Right_Line_Tracker_Dark = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Right_Line_Tracker_Light_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Line_Tracker_Dark_Address_H);
  ui_Right_Line_Tracker_Light = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Left_Motor_Offset_Address_L);
  b_HighByte = EEPROM.read(ci_Left_Motor_Offset_Address_H);
  ui_Left_Motor_Offset = word(b_HighByte, b_LowByte);
  b_LowByte = EEPROM.read(ci_Right_Motor_Offset_Address_L);
  b_HighByte = EEPROM.read(ci_Right_Motor_Offset_Address_H);
  ui_Right_Motor_Offset = word(b_HighByte, b_LowByte);

  //attach servo motors being used
  servo_turntable.attach(ci_servo_turntable);
  servo_pivot.attach(ci_servo_pivot);
  servo_arm.attach(ci_servo_arm);
  servo_magnet.attach(ci_servo_magnet);


}
/*********************************************************************END SETUP**************************************************************/

int previous = 0;
int current = 0;
int previous_ping = 0;
int current_ping = 0;
int previous_scan = 0;
int current_scan = 0;

bool drive = true;
int bot_speed = 1300;
int bot_stop = 1500;
bool start_turn = false;//used to indicate when turn has started or not
bool end_turn = true;//used to indicate when turn has ended or not
int num_turns = 0;
int tesseract_count = 0;
int left_wheel = 0;//used to calculated wheel rotation while turning
int right_wheel = 0;//used to calculated wheel rotation while turning
int left_wheel_prev = 0;
int right_wheel_prev = 0;

//use these variables for navigation (mapping the area)
int x = 0;
int y = 0;

void readLineTrackers()
{
  ui_Left_Line_Tracker_Data = analogRead(ci_Left_Line_Tracker);
  ui_Middle_Line_Tracker_Data = analogRead(ci_Middle_Line_Tracker);
  ui_Right_Line_Tracker_Data = analogRead(ci_Right_Line_Tracker);

  /*if (ui_Left_Line_Tracker_Data < (ui_Left_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
    CharliePlexM::Write(ci_Left_Line_Tracker_LED, HIGH);s
    }
    else
    {
    CharliePlexM::Write(ci_Left_Line_Tracker_LED, LOW);
    }
    if (ui_Middle_Line_Tracker_Data < (ui_Middle_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
    CharliePlexM::Write(ci_Middle_Line_Tracker_LED, HIGH);
    }
    else
    {
    CharliePlexM::Write(ci_Middle_Line_Tracker_LED, LOW);
    }
    if (ui_Right_Line_Tracker_Data < (ui_Right_Line_Tracker_Dark - ui_Line_Tracker_Tolerance))
    {
    CharliePlexM::Write(ci_Right_Line_Tracker_LED, HIGH);
    }
    else
    {
    CharliePlexM::Write(ci_Right_Line_Tracker_LED, LOW);
    }*/

#ifdef DEBUG_LINE_TRACKERS
  Serial.print("Trackers: Left = ");
  Serial.print(ui_Left_Line_Tracker_Data, DEC);
  Serial.print(", Middle = ");
  Serial.print(ui_Middle_Line_Tracker_Data, DEC);
  Serial.print(", Right = ");
  Serial.println(ui_Right_Line_Tracker_Data, DEC);
#endif

}

// set mode indicator LED state
/*void Indicator()
  {
  //display routine, if true turn on led
  CharliePlexM::Write(ci_Indicator_LED, !(ui_Mode_Indicator[ui_Mode_Indicator_Index] &
                                          (iArray[iArrayIndex])));
  iArrayIndex++;
  iArrayIndex = iArrayIndex & 15;

  // read values from line trackers and update status of line tracker LEDs

  }*/

bool clockwise = true;
int pos = 0;

void scan() {//rotates arm to scan for good tesseracks
  if (clockwise == false) {

    pos++;
    servo_turntable.write(pos);
    if (pos >= 180)
      clockwise = true;
    /*
        for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
          // in steps of 1 degree
          servo_turntable.write(pos);              // tell servo to go to position in variable 'pos'
          delay(15);                       // waits 15ms for the servo to reach the position
        }
        clockwise = true;
    */
  }
  else {
    pos--;
    servo_turntable.write(pos);
    if (pos <= 0)
      clockwise = false;
    /*
        for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
          servo_turntable.write(pos);              // tell servo to go to position in variable 'pos'
          delay(15);                       // waits 15ms for the servo to reach the position
        }
        clockwise = false;
    */
  }
}

void Ping_Left()
{
  //Ping Ultrasonic
  //Send the Ultrasonic Range Finder a 10 microsecond pulse per tech spec
  digitalWrite(ci_Ultrasonic_Ping_Left, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_Ultrasonic_Ping_Left, LOW);
  //use command pulseIn to listen to Ultrasonic_Data pin to record the
  //time that it takes from when the Pin goes HIGH until it goes LOW
  ul_Echo_Time_Left = pulseIn(ci_Ultrasonic_Data_Left, HIGH, 10000);


#ifdef DEBUG_ULTRASONIC_LEFT
  // Print Sensor Readings
  Serial.print("Time (microseconds): ");
  Serial.print(ul_Echo_Time_Left, DEC);
  Serial.print(", cm: ");
  Serial.println(ul_Echo_Time_Left / 24); //divide time by 58 to get distance in cm
#endif

}

void Ping_Right()
{
  //Ping Ultrasonic
  //Send the Ultrasonic Range Finder a 10 microsecond pulse per tech spec
  digitalWrite(ci_Ultrasonic_Ping_Right, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_Ultrasonic_Ping_Right, LOW);
  //use command pulseIn to listen to Ultrasonic_Data pin to record the
  //time that it takes from when the Pin goes HIGH until it goes LOW
  ul_Echo_Time_Right = pulseIn(ci_Ultrasonic_Data_Right, HIGH, 10000);


#ifdef DEBUG_ULTRASONIC_RIGHT
  // Print Sensor Readings
  Serial.print("Time (microseconds): ");
  Serial.print(ul_Echo_Time_Right, DEC);
  Serial.print(", cm: ");
  Serial.println(ul_Echo_Time_Right / 24); //divide time by 58 to get distance in cm
#endif

}

void Ping_Front()
{
  //Ping Ultrasonic
  //Send the Ultrasonic Range Finder a 10 microsecond pulse per tech spec
  digitalWrite(ci_Ultrasonic_Ping_Front, HIGH);
  delayMicroseconds(10);  //The 10 microsecond pause where the pulse in "high"
  digitalWrite(ci_Ultrasonic_Ping_Front, LOW);
  //use command pulseIn to listen to Ultrasonic_Data pin to record the
  //time that it takes from when the Pin goes HIGH until it goes LOW
  ul_Echo_Time_Front = pulseIn(ci_Ultrasonic_Data_Front, HIGH, 10000);

#ifdef DEBUG_ULTRASONIC_FRONT
  // Print Sensor Readings
  Serial.print("Time (microseconds): ");
  Serial.print(ul_Echo_Time_Front, DEC);
  Serial.print(", cm: ");
  Serial.println(ul_Echo_Time_Front / 24); //divide time by 58 to get distance in cm
#endif
  //indicate LED on Charlieplex when reached below 20 cm
  /*if ((ul_Echo_Time_Front / 24) < 20) {
    CharliePlexM::Write(12, HIGH);
    }
    else {
    CharliePlexM::Write(12, LOW);
    }*/

}

void travel() {//makes robot travel straight, main movement function of robot

  if (num_turns % 2 == 0) {//use LEFT ultrasonic to detect wall and travel in a straight path
    if ((ul_Echo_Time_Left / 24) >= (10.5 + (25 * num_turns))) { //if moving AWAY FROM wall turn left
      servo_LeftMotor.writeMicroseconds(bot_speed + 10); //left motor moves slower
      servo_RightMotor.writeMicroseconds(bot_speed);
    }
    else if ((ul_Echo_Time_Left / 24) <= (9.5 + (25 * num_turns))) { //if moving TOWARD wall, turn right
      servo_LeftMotor.writeMicroseconds(bot_speed);
      servo_RightMotor.writeMicroseconds(bot_speed + 1); //right motor moves slower. This is small because robot leans to the right
    }
    else {//when in correct position do not adjust speed
      servo_LeftMotor.writeMicroseconds(bot_speed);
      servo_RightMotor.writeMicroseconds(bot_speed);
    }
  }

  else { //use RIGHT ultrasonic to detect wall and travel in a straight path
    if ((ul_Echo_Time_Right / 24) >= (10.5 + (25 * num_turns))) { //if moving TOWARD wall, turn right
      servo_LeftMotor.writeMicroseconds(bot_speed);
      servo_RightMotor.writeMicroseconds(bot_speed + 10); //right motor moves slower
    }
    else if ((ul_Echo_Time_Right / 24) <= (9.5 + (25 * num_turns))) { //if moving AWAY FROM wall, turn left
      servo_LeftMotor.writeMicroseconds(bot_speed + 10); //left motor moves slower
      servo_RightMotor.writeMicroseconds(bot_speed);
    }
    else {//when in correct position do not adjust speed
      servo_LeftMotor.writeMicroseconds(bot_speed);
      servo_RightMotor.writeMicroseconds(bot_speed);
    }
  }

  /*if ((current - previous) > 1000) { //stop after one second of movement
    servo_LeftMotor.writeMicroseconds(1500);
    servo_RightMotor.writeMicroseconds(1500);
    current = millis();
    previous = current;
    drive = false;
    }*/

}

void turn() {//turns on either end when it reaches the end wall
  left_wheel = encoder_LeftMotor.getRawPosition();
  right_wheel = encoder_RightMotor.getRawPosition();

  if (num_turns % 2 == 0) {
    servo_LeftMotor.writeMicroseconds(bot_speed);
    servo_RightMotor.writeMicroseconds(bot_stop);
  }
  else {
    servo_LeftMotor.writeMicroseconds(bot_stop);
    servo_RightMotor.writeMicroseconds(bot_speed);
  }

  if ((abs(left_wheel - left_wheel_prev) >= 1800) || (abs(right_wheel - right_wheel_prev) >= 1800)) { //when 180 degree turn is complete
    start_turn = false;
    end_turn = true;
    num_turns++;
    //encoder_LeftMotor.zero();
    //encoder_RightMotor.zero();
    //x++;
  }

  /*if ((current - previous) > 1000) { //stop after one second of movement
    servo_LeftMotor.writeMicroseconds(1500);
    servo_RightMotor.writeMicroseconds(1500);
    current = millis();
    previous = current;
    drive = false;
    }*/

}

void GoHome() { //after tesserack is collected, goes home to place tesseract and then returns to previous position
  left_wheel = encoder_LeftMotor.getRawPosition();
  right_wheel = encoder_RightMotor.getRawPosition();
  //need to add code to obtain left_wheel_prev.....same for right wheel********

  if (num_turns % 2 == 0) {//use LEFT ultrasonic to detect wall and travel in a straight path
    servo_LeftMotor.writeMicroseconds(bot_stop + 100);//moves left wheel backward
    servo_RightMotor.writeMicroseconds(bot_stop - 100);//moves right wheel forward
    //turn 90 degrees left ward
  }

  else { //use RIGHT ultrasonic to detect wall and travel in a straight path

    servo_LeftMotor.writeMicroseconds(bot_stop - 100);//moves left wheel forward
    servo_RightMotor.writeMicroseconds(bot_stop + 100);//moves right wheel backward
    //turn 90 degrees right
  }
  //do two 90 degree turns to go home....900 might not be correct
  if ((abs(left_wheel - left_wheel_prev) >= 900) || (abs(right_wheel - right_wheel_prev) >= 900)) { //when quarter turn is complete might need to adjust value here
    servo_LeftMotor.writeMicroseconds(bot_stop);
    servo_RightMotor.writeMicroseconds(bot_stop);
  }

  /*if (ul_Echo_Time_Front / 24) <= 35) {//if ultrasonic detects wall in front of it 35 cm away
    //turn 90 degree leftward
    servo_LeftMotor.writeMicroseconds(bot_stop + 100);//moves left wheel backward
    servo_RightMotor.writeMicroseconds(bot_stop - 100);//moves right wheel forward
    }*/

}

/************************************************************* MAIN LOOP *****************************************************************/
void loop() {
  //servo_arm.write(180);
  //servo_pivot.write(70);



  if ((millis() - ul_3_Second_timer) > 3000)
  {
    bt_3_S_Time_Up = true;
  }

  // button-based mode selection
  if (CharliePlexM::ui_Btn)
  {
    if (bt_Do_Once == false)
    {
      bt_Do_Once = true;
      ui_Robot_State_Index++;
      ui_Robot_State_Index = ui_Robot_State_Index & 7;
      ul_3_Second_timer = millis();
      bt_3_S_Time_Up = false;
      bt_Cal_Initialized = false;
      ui_Cal_Cycle = 0;
    }
  }
  else
  {
    bt_Do_Once = LOW;
  }

  // check if drive motors should be powered
  bt_Motors_Enabled = digitalRead(ci_Motor_Enable_Switch);

  // modes
  // 0 = default after power up/reset
  // 1 = Press mode button once to enter. Run robot.
  // 2 = Press mode button twice to enter. Calibrate line tracker light level.
  // 3 = Press mode button three times to enter. Calibrate line tracker dark level.
  // 4 = Press mode button four times to enter. Calibrate motor speeds to drive straight.
  switch (ui_Robot_State_Index)
  {
    case 0:    //Robot stopped
      {
        readLineTrackers();
        Ping_Front();
        servo_LeftMotor.writeMicroseconds(ci_Left_Motor_Stop);
        servo_RightMotor.writeMicroseconds(ci_Right_Motor_Stop);
        encoder_LeftMotor.zero();
        encoder_RightMotor.zero();
        ui_Mode_Indicator_Index = 0;
        break;
      }

    case 1:    //Robot Run after 3 seconds
      {
        if (bt_3_S_Time_Up)
        {
          readLineTrackers();

#ifdef DEBUG_ENCODERS
          l_Left_Motor_Position = encoder_LeftMotor.getRawPosition();
          l_Right_Motor_Position = encoder_RightMotor.getRawPosition();

          Serial.print("Encoders L: ");
          Serial.print(l_Left_Motor_Position);
          Serial.print(", R: ");
          Serial.println(l_Right_Motor_Position);
#endif

          // set motor speeds
          ui_Left_Motor_Speed = constrain(ui_Motors_Speed + ui_Left_Motor_Offset, 1600, 2100);
          ui_Right_Motor_Speed = constrain(ui_Motors_Speed + ui_Right_Motor_Offset, 1600, 2100);

          /*****************************************************************************************************************************************
            Main operation code HERE
            Implementation of mode 1 operations of MSE 2202 Project

            /**************************************************************************************************************************************/
          current_scan = millis();
          if ((current_scan - previous_scan) > 15) {
            scan();
            previous_scan = current_scan;
          }
          /*
                    current = millis();
                    current_ping = millis();

                    if ((current_ping - previous_ping) > 500) {
                      Ping_Front();
                      Ping_Right();
                      Ping_Left();
                      previous_ping = current_ping;
                    }

                    if (drive == true) {//drive forward
                      //CharliePlexM::Write(3, HIGH);

                      if (((ul_Echo_Time_Front / 24) <= 35) && (end_turn == true)) {
                        start_turn = true;
                        left_wheel_prev = encoder_LeftMotor.getRawPosition();
                        right_wheel_prev = encoder_RightMotor.getRawPosition();
                        end_turn = false;
                      }
                      if (start_turn == true) {
                        turn();
                      }
                      //insert if statement for when tessarct is picked up to initiate going home
                      //insert if statement in case front line trackers detect that we have reached the neutral zone
                      else {
                        travel();
                      }

                    }

                    else if (drive == false) {
                      //CharliePlexM::Write(3, LOW);
                      scan();
                      y++;
                      drive = true;//after finished scanning continue driving
                      current = millis();
                      previous = current;
                    }
          */



#ifdef DEBUG_MOTORS
          Serial.print("Motors enabled: ");
          Serial.print(bt_Motors_Enabled);
          Serial.print(", Default: ");
          Serial.print(ui_Motors_Speed);
          Serial.print(", Left = ");
          Serial.print(ui_Left_Motor_Speed);
          Serial.print(", Right = ");
          Serial.println(ui_Right_Motor_Speed);
#endif
          ui_Mode_Indicator_Index = 1;
        }
        break;
      }

    /*case 2:    //Calibrate line tracker light levels after 3 seconds
      {
        if (bt_3_S_Time_Up)
        {
          if (!bt_Cal_Initialized)
          {
            bt_Cal_Initialized = true;
            ui_Left_Line_Tracker_Light = 0;
            ui_Middle_Line_Tracker_Light = 0;
            ui_Right_Line_Tracker_Light = 0;
            ul_Calibration_Time = millis();
            ui_Cal_Count = 0;
          }
          else if ((millis() - ul_Calibration_Time) > ci_Line_Tracker_Calibration_Interval)
          {
            ul_Calibration_Time = millis();
            readLineTrackers();
            ui_Left_Line_Tracker_Light += ui_Left_Line_Tracker_Data;
            ui_Middle_Line_Tracker_Light += ui_Middle_Line_Tracker_Data;
            ui_Right_Line_Tracker_Light += ui_Right_Line_Tracker_Data;
            ui_Cal_Count++;
          }
          if (ui_Cal_Count == ci_Line_Tracker_Cal_Measures)
          {
            ui_Left_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
            ui_Middle_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
            ui_Right_Line_Tracker_Light /= ci_Line_Tracker_Cal_Measures;
      #ifdef DEBUG_LINE_TRACKER_CALIBRATION
            Serial.print("Light Levels: Left = ");
            Serial.print(ui_Left_Line_Tracker_Light, DEC);
            Serial.print(", Middle = ");
            Serial.print(ui_Middle_Line_Tracker_Light, DEC);
            Serial.print(", Right = ");
            Serial.println(ui_Right_Line_Tracker_Light, DEC);
      #endif
            EEPROM.write(ci_Left_Line_Tracker_Light_Address_L, lowByte(ui_Left_Line_Tracker_Light));
            EEPROM.write(ci_Left_Line_Tracker_Light_Address_H, highByte(ui_Left_Line_Tracker_Light));
            EEPROM.write(ci_Middle_Line_Tracker_Light_Address_L, lowByte(ui_Middle_Line_Tracker_Light));
            EEPROM.write(ci_Middle_Line_Tracker_Light_Address_H, highByte(ui_Middle_Line_Tracker_Light));
            EEPROM.write(ci_Right_Line_Tracker_Light_Address_L, lowByte(ui_Right_Line_Tracker_Light));
            EEPROM.write(ci_Right_Line_Tracker_Light_Address_H, highByte(ui_Right_Line_Tracker_Light));
            ui_Robot_State_Index = 0;    // go back to Mode 0
          }
          ui_Mode_Indicator_Index = 2;
        }
        break;
      }

      case 3:    // Calibrate line tracker dark levels after 3 seconds
      {
        if (bt_3_S_Time_Up)
        {
          if (!bt_Cal_Initialized)
          {
            bt_Cal_Initialized = true;
            ui_Left_Line_Tracker_Dark = 0;
            ui_Middle_Line_Tracker_Dark = 0;
            ui_Right_Line_Tracker_Dark = 0;
            ul_Calibration_Time = millis();
            ui_Cal_Count = 0;
          }
          else if ((millis() - ul_Calibration_Time) > ci_Line_Tracker_Calibration_Interval)
          {
            ul_Calibration_Time = millis();
            readLineTrackers();
            ui_Left_Line_Tracker_Dark += ui_Left_Line_Tracker_Data;
            ui_Middle_Line_Tracker_Dark += ui_Middle_Line_Tracker_Data;
            ui_Right_Line_Tracker_Dark += ui_Right_Line_Tracker_Data;
            ui_Cal_Count++;
          }
          if (ui_Cal_Count == ci_Line_Tracker_Cal_Measures)
          {
            ui_Left_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
            ui_Middle_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
            ui_Right_Line_Tracker_Dark /= ci_Line_Tracker_Cal_Measures;
      #ifdef DEBUG_LINE_TRACKER_CALIBRATION
            Serial.print("Dark Levels: Left = ");
            Serial.print(ui_Left_Line_Tracker_Dark, DEC);
            Serial.print(", Middle = ");
            Serial.print(ui_Middle_Line_Tracker_Dark, DEC);
            Serial.print(", Right = ");
            Serial.println(ui_Right_Line_Tracker_Dark, DEC);
      #endif
            EEPROM.write(ci_Left_Line_Tracker_Dark_Address_L, lowByte(ui_Left_Line_Tracker_Dark));
            EEPROM.write(ci_Left_Line_Tracker_Dark_Address_H, highByte(ui_Left_Line_Tracker_Dark));
            EEPROM.write(ci_Middle_Line_Tracker_Dark_Address_L, lowByte(ui_Middle_Line_Tracker_Dark));
            EEPROM.write(ci_Middle_Line_Tracker_Dark_Address_H, highByte(ui_Middle_Line_Tracker_Dark));
            EEPROM.write(ci_Right_Line_Tracker_Dark_Address_L, lowByte(ui_Right_Line_Tracker_Dark));
            EEPROM.write(ci_Right_Line_Tracker_Dark_Address_H, highByte(ui_Right_Line_Tracker_Dark));
            ui_Robot_State_Index = 0;    // go back to Mode 0
          }
          ui_Mode_Indicator_Index = 3;
        }
        break;
      }*/

    case 4:    //Calibrate motor straightness after 3 seconds.
      {
        if (bt_3_S_Time_Up)
        {
          if (!bt_Cal_Initialized)
          {
            bt_Cal_Initialized = true;
            encoder_LeftMotor.zero();
            encoder_RightMotor.zero();
            ul_Calibration_Time = millis();
            servo_LeftMotor.writeMicroseconds(ui_Motors_Speed);
            servo_RightMotor.writeMicroseconds(ui_Motors_Speed);
          }
          else if ((millis() - ul_Calibration_Time) > ci_Motor_Calibration_Time)
          {
            servo_LeftMotor.writeMicroseconds(ci_Left_Motor_Stop);
            servo_RightMotor.writeMicroseconds(ci_Right_Motor_Stop);
            l_Left_Motor_Position = encoder_LeftMotor.getRawPosition();
            l_Right_Motor_Position = encoder_RightMotor.getRawPosition();
            if (l_Left_Motor_Position > l_Right_Motor_Position)
            {
              // May have to update this if different calibration time is used
              ui_Right_Motor_Offset = 0;
              ui_Left_Motor_Offset = (l_Left_Motor_Position - l_Right_Motor_Position) / 4;
            }
            else
            {
              // May have to update this if different calibration time is used
              ui_Right_Motor_Offset = (l_Right_Motor_Position - l_Left_Motor_Position) / 4;
              ui_Left_Motor_Offset = 0;
            }

#ifdef DEBUG_MOTOR_CALIBRATION
            Serial.print("Motor Offsets: Left = ");
            Serial.print(ui_Left_Motor_Offset);
            Serial.print(", Right = ");
            Serial.println(ui_Right_Motor_Offset);
#endif
            EEPROM.write(ci_Right_Motor_Offset_Address_L, lowByte(ui_Right_Motor_Offset));
            EEPROM.write(ci_Right_Motor_Offset_Address_H, highByte(ui_Right_Motor_Offset));
            EEPROM.write(ci_Left_Motor_Offset_Address_L, lowByte(ui_Left_Motor_Offset));
            EEPROM.write(ci_Left_Motor_Offset_Address_H, highByte(ui_Left_Motor_Offset));

            ui_Robot_State_Index = 0;    // go back to Mode 0
          }
#ifdef DEBUG_MOTOR_CALIBRATION
          Serial.print("Encoders L: ");
          Serial.print(encoder_LeftMotor.getRawPosition());
          Serial.print(", R: ");
          Serial.println(encoder_RightMotor.getRawPosition());
#endif
          ui_Mode_Indicator_Index = 4;
        }
        break;
      }
  }

  if ((millis() - ul_Display_Time) > ci_Display_Time)
  {
    ul_Display_Time = millis();

#ifdef DEBUG_MODE_DISPLAY
    Serial.print("Mode: ");
    Serial.println(ui_Mode_Indicator[ui_Mode_Indicator_Index], DEC);
#endif
    /*bt_Heartbeat = !bt_Heartbeat;
      CharliePlexM::Write(ci_Heartbeat_LED, bt_Heartbeat);
      digitalWrite(13, bt_Heartbeat);
      Indicator();*/
  }
}


/******************************************************************* END MAIN LOOP ******************************************************/


