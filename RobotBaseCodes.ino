/*
  MechEng 706 Base Code

  This code provides basic movement and sensor reading for the MechEng 706 Mecanum Wheel Robot Project

  Hardware:
    Arduino Mega2560 https://www.arduino.cc/en/Guide/ArduinoMega2560
    MPU-9250 https://www.sparkfun.com/products/13762
    Ultrasonic Sensor - HC-SR04 https://www.sparkfun.com/products/13959
    Infrared Proximity Sensor - Sharp https://www.sparkfun.com/products/242
    Infrared Proximity Sensor Short Range - Sharp https://www.sparkfun.com/products/12728
    Servo - Generic (Sub-Micro Size) https://www.sparkfun.com/products/9065
    Vex Motor Controller 29 https://www.vexrobotics.com/276-2193.html
    Vex Motors https://www.vexrobotics.com/motors.html
    Turnigy nano-tech 2200mah 2S https://hobbyking.com/en_us/turnigy-nano-tech-2200mah-2s-25-50c-lipo-pack.html

  Date: 11/11/2016
  Author: Logan Stuart
  Modified: 15/02/2018
  Author: Logan Stuart
*/
#include <Servo.h> //Need for Servo pulse output
#include <SoftwareSerial.h>

// #define NO_READ_GYRO  //Uncomment of GYRO is not attached.
// #define NO_HC-SR04 //Uncomment of HC-SR04 ultrasonic ranging sensor is not attached.
// #define NO_BATTERY_V_OK //Uncomment of BATTERY_V_OK if you do not care about battery damage.

// State machine states
enum STATE
{
  INITIALISING,
  RUNNING,
  STOPPED
};

#define BLUETOOTH_RX 10
// Serial Data output pin
#define BLUETOOTH_TX 11

SoftwareSerial BluetoothSerial(BLUETOOTH_RX, BLUETOOTH_TX);

// Refer to Shield Pinouts.jpg for pin locations

// Default motor control pins
const byte left_front = 46;
const byte left_rear = 47;
const byte right_rear = 50;
const byte right_front = 51;

// Default ultrasonic ranging sensor pins, these pins are defined my the Shield
const int TRIG_PIN = 48;
const int ECHO_PIN = 49;

// Default IR sensor pins, these pins are defined by the Shield
#define IR_41_01 12
#define IR_41_02 13
#define IR_41_03 13
// #define IR_2Y_01 14
#define IR_2Y_02 14
// uncomment if these IR sensors are used
//  #define IR_2Y_03 14
#define IR_2Y_04 15

// Anything over 400 cm (23200 us pulse) is "out of range". Hit:If you decrease to this the ranging sensor but the timeout is short, you may not need to read up to 4meters.
const unsigned int MAX_DIST = 23200;

Servo left_font_motor;  // create servo object to control Vex Motor Controller 29
Servo left_rear_motor;  // create servo object to control Vex Motor Controller 29
Servo right_rear_motor; // create servo object to control Vex Motor Controller 29
Servo right_font_motor; // create servo object to control Vex Motor Controller 29
Servo turret_motor;

int speed_val = 300;
int speed_change;

// gyro
int sensorPin = A2;            // define the pin that gyro is connected
int sensorValue = 0;           // read out value of sensor
float gyroSupplyVoltage = 5;   // supply voltage for gyro
float gyroZeroVoltage = 0;     // the value of voltage when gyro is zero
float gyroSensitivity = 0.007; // gyro sensitivity unit is (mv/degree/second) get from datasheet
float rotationThreshold = 1.5; // because of gyro drifting, defining rotation angular velocity less than
// this value will not be ignored
float gyroRate = 0;     // read out value of sensor in voltage
float currentAngle = 0; // current angle calculated by angular velocity integral on

// Serial Pointer
HardwareSerial *SerialCom;

int pos = 0;

  int i;
  float sum = 0;
void setup(void)
{
  turret_motor.attach(11);
  pinMode(LED_BUILTIN, OUTPUT);

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Setup the Serial port and pointer, the pointer allows switching the debug info through the USB port(Serial) or Bluetooth port(Serial1) with ease.
  SerialCom = &Serial;
  SerialCom->begin(115200);
  SerialCom->println("MECHENG706_Base_Code_25/01/2018");

  BluetoothSerial.begin(115200);


  delay(1000);
  SerialCom->println("Setup....");

  // // setting up gyro

  // pinMode(sensorPin, INPUT);
  // Serial.println("please keep the sensor still for calibration");
  // Serial.println("get the gyro zero voltage");
  // for (i = 0; i < 100; i++) // read 100 values of voltage when gyro is at still, to calculate the zero-drift
  // {
  //   sensorValue = analogRead(sensorPin);
  //   sum += sensorValue;
  //   delay(5);
  // }
  // gyroZeroVoltage = sum / 100; // average the sum as the zero drifting

  delay(1000); // settling time but no really needed
}

void loop(void) // main loop
{
  static STATE machine_state = INITIALISING;
  //Finite-state machine Code
  switch (machine_state) {
    case INITIALISING:
      machine_state = initialising();
      break;
    case RUNNING: //Lipo Battery Volage OK
      //machine_state =  running();
      //SerialCom->println(String(currentAngle));
      //SerialCom->print(String(currentAngle));
      //SerialCom->print("\n");
      

      driveStrightWithGyro(6000);
      delay(5);
      break;
    case STOPPED: //Stop of Lipo Battery voltage is too low, to protect Battery
      machine_state =  stopped();
      break;
  };

}

STATE initialising()
{
  // initialising
  SerialCom->println("INITIALISING....");
  //delay(1000); // One second delay to see the serial String "INITIALISING...."
  SerialCom->println("Enabling Motors...");
  enable_motors();
  SerialCom->println("RUNNING STATE...");

// setting up gyro
  //int i;
  //float sum = 0;
  // pinMode(sensorPin, INPUT);
  Serial.println("please keep the sensor still for calibration");
  Serial.println("get the gyro zero voltage");
  for (i = 0; i < 100; i++) // read 100 values of voltage when gyro is at still, to calculate the zero-drift
  {
    sensorValue = analogRead(sensorPin);
    sum += sensorValue;
    delay(5);
  }
  gyroZeroVoltage = sum / 100; // average the sum as the zero drifting





  return RUNNING;
}

STATE running()
{

  static unsigned long previous_millis;

  read_serial_command();
  fast_flash_double_LED_builtin();

  if (millis() - previous_millis > 500)
  { // Arduino style 500ms timed execution statement
    previous_millis = millis();

    SerialCom->println("RUNNING---------");
    speed_change_smooth();
    Analog_Range_A4();

#ifndef NO_READ_GYRO
    GYRO_reading();
#endif

#ifndef NO_HC - SR04
    HC_SR04_range();
#endif

#ifndef NO_BATTERY_V_OK
    if (!is_battery_voltage_OK())
      return STOPPED;
#endif

    turret_motor.write(pos);

    if (pos == 0)
    {
      pos = 45;
    }
    else
    {
      pos = 0;
    }
  }

  return RUNNING;
}

// Stop of Lipo Battery voltage is too low, to protect Battery
STATE stopped()
{
  static byte counter_lipo_voltage_ok;
  static unsigned long previous_millis;
  int Lipo_level_cal;
  disable_motors();
  slow_flash_LED_builtin();

  if (millis() - previous_millis > 500)
  { // print massage every 500ms
    previous_millis = millis();
    SerialCom->println("STOPPED---------");

#ifndef NO_BATTERY_V_OK
    // 500ms timed if statement to check lipo and output speed settings
    if (is_battery_voltage_OK())
    {
      SerialCom->print("Lipo OK waiting of voltage Counter 10 < ");
      SerialCom->println(counter_lipo_voltage_ok);
      counter_lipo_voltage_ok++;
      if (counter_lipo_voltage_ok > 10)
      { // Making sure lipo voltage is stable
        counter_lipo_voltage_ok = 0;
        enable_motors();
        SerialCom->println("Lipo OK returning to RUN STATE");
        return RUNNING;
      }
    }
    else
    {
      counter_lipo_voltage_ok = 0;
    }
#endif
  }
  return STOPPED;
}

void fast_flash_double_LED_builtin()
{
  static byte indexer = 0;
  static unsigned long fast_flash_millis;
  if (millis() > fast_flash_millis)
  {
    indexer++;
    if (indexer > 4)
    {
      fast_flash_millis = millis() + 700;
      digitalWrite(LED_BUILTIN, LOW);
      indexer = 0;
    }
    else
    {
      fast_flash_millis = millis() + 100;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
}

void slow_flash_LED_builtin()
{
  static unsigned long slow_flash_millis;
  if (millis() - slow_flash_millis > 2000)
  {
    slow_flash_millis = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void speed_change_smooth()
{
  speed_val += speed_change;
  if (speed_val > 1000)
    speed_val = 1000;
  speed_change = 0;
}

#ifndef NO_BATTERY_V_OK
boolean is_battery_voltage_OK()
{
  static byte Low_voltage_counter;
  static unsigned long previous_millis;

  int Lipo_level_cal;
  int raw_lipo;
  // the voltage of a LiPo cell depends on its chemistry and varies from about 3.5V (discharged) = 717(3.5V Min) https://oscarliang.com/lipo-battery-guide/
  // to about 4.20-4.25V (fully charged) = 860(4.2V Max)
  // Lipo Cell voltage should never go below 3V, So 3.5V is a safety factor.
  raw_lipo = analogRead(A0);
  Lipo_level_cal = (raw_lipo - 717);
  Lipo_level_cal = Lipo_level_cal * 100;
  Lipo_level_cal = Lipo_level_cal / 143;

  if (Lipo_level_cal > 0 && Lipo_level_cal < 160)
  {
    previous_millis = millis();
    SerialCom->print("Lipo level:");
    SerialCom->print(Lipo_level_cal);
    SerialCom->print("%");
    // SerialCom->print(" : Raw Lipo:");
    // SerialCom->println(raw_lipo);
    SerialCom->println("");
    Low_voltage_counter = 0;
    return true;
  }
  else
  {
    if (Lipo_level_cal < 0)
      SerialCom->println("Lipo is Disconnected or Power Switch is turned OFF!!!");
    else if (Lipo_level_cal > 160)
      SerialCom->println("!Lipo is Overchanged!!!");
    else
    {
      SerialCom->println("Lipo voltage too LOW, any lower and the lipo with be damaged");
      SerialCom->print("Please Re-charge Lipo:");
      SerialCom->print(Lipo_level_cal);
      SerialCom->println("%");
    }

    Low_voltage_counter++;
    if (Low_voltage_counter > 5)
      return false;
    else
      return true;
  }
}
#endif

#ifndef NO_HC - SR04
void HC_SR04_range()
{
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float cm;
  float inches;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  t1 = micros();
  while (digitalRead(ECHO_PIN) == 0)
  {
    t2 = micros();
    pulse_width = t2 - t1;
    if (pulse_width > (MAX_DIST + 1000))
    {
      SerialCom->println("HC-SR04: NOT found");
      return;
    }
  }

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min

  t1 = micros();
  while (digitalRead(ECHO_PIN) == 1)
  {
    t2 = micros();
    pulse_width = t2 - t1;
    if (pulse_width > (MAX_DIST + 1000))
    {
      SerialCom->println("HC-SR04: Out of range");
      return;
    }
  }

  t2 = micros();
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed
  // of sound in air at sea level (~340 m/s).
  cm = pulse_width / 58.0;
  inches = pulse_width / 148.0;

  // Print out results
  if (pulse_width > MAX_DIST)
  {
    SerialCom->println("HC-SR04: Out of range");
  }
  else
  {
    SerialCom->print("HC-SR04:");
    SerialCom->print(cm);
    SerialCom->println("cm");
  }
}
#endif

void Analog_Range_A4()
{
  SerialCom->print("Analog Range A4:");
  SerialCom->println(analogRead(A4));
}

#ifndef NO_READ_GYRO
void GYRO_reading()
{
  SerialCom->print("GYRO A3:");
  SerialCom->println(analogRead(A3));
}
#endif

// Serial command pasing
void read_serial_command()
{
  if (SerialCom->available())
  {
    char val = SerialCom->read();
    SerialCom->print("Speed:");
    SerialCom->print(speed_val);
    SerialCom->print(" ms ");

    // Perform an action depending on the command
    switch (val)
    {
    case 'w': // Move Forward
    case 'W':
      forward();
      SerialCom->println("Forward");
      break;
    case 's': // Move Backwards
    case 'S':
      reverse();
      SerialCom->println("Backwards");
      break;
    case 'q': // Turn Left
    case 'Q':
      strafe_left();
      SerialCom->println("Strafe Left");
      break;
    case 'e': // Turn Right
    case 'E':
      strafe_right();
      SerialCom->println("Strafe Right");
      break;
    case 'a': // Turn Right
    case 'A':
      ccw();
      SerialCom->println("ccw");
      break;
    case 'd': // Turn Right
    case 'D':
      cw();
      SerialCom->println("cw");
      break;
    case '-': // Turn Right
    case '_':
      speed_change = -100;
      SerialCom->println("-100");
      break;
    case '=':
    case '+':
      speed_change = 100;
      SerialCom->println("+");
      break;
    default:
      stop();
      SerialCom->println("stop");
      break;
    }
  }
}

//----------------------Sensor Reading & conversion to mm-------------------------

double IR_sensorReadDistance(String sensor)
// input can be : "41_01", "04_02", "02_01", "02_02", "02_03", "02_04",
// return distance in mm
{
  double distance;
  double sensor_value;
  if (sensor == "41_01")
  {
    sensor_value = analogRead(IR_41_01);
    distance = 27592 * pow(sensor_value, -1.018);
  }
  else if (sensor == "41_02")
  {
    sensor_value = analogRead(IR_41_02);
    distance = 7935.4 * pow(sensor_value, -0.827);
  }
  else if (sensor == "41_03")
  {
    sensor_value = analogRead(IR_41_03);
    distance = 30119 * pow(sensor_value, -1.039);
  }
  // else if(sensor == "2Y_01")
  // {
  //   sensor_value = analogRead(IR_2Y_01);
  //   distance = 1888777 * pow(sensor_value, -1.237);
  // }
  else if (sensor == "2Y_02")
  {
    sensor_value = analogRead(IR_2Y_02);
    distance = 92838 * pow(sensor_value, -1.097);
  }
  // else if(sensor = "2Y_03")
  // {
  //   sensor_value = analogRead(IR_2Y_03);
  //   distance = 7927.4 * pow(sensor_value, -0.687);
  // }
  else if (sensor = "2Y_04")
  {
    sensor_value = analogRead(IR_2Y_04);
    distance = 50857 * pow(sensor_value, -0.994);
  }
  else
  {
    SerialCom->println("Invalid sensor");
    distance = 0;
  }
  return distance;
}

//-------------------------------Car Movement-----------------------------------
void Car_Move_withIRSensor(String left_front_IR, String left_back_IR, String right_front_IR, String right_back_IR)
{
  float torlance = 3; // mm

  float left_front_distance = IR_sensorReadDistance(left_front_IR);
  float left_back_distance = IR_sensorReadDistance(left_back_IR);
  float right_front_distance = IR_sensorReadDistance(right_front_IR);
  float right_back_distance = IR_sensorReadDistance(right_back_IR);

  // demo only, only work with left sensor
  // need to be modified after logic of car movement is determined
  if (left_front_distance - left_back_distance > torlance)
  {
    ccw();
  }
  else if (left_back_distance - left_front_distance > torlance)
  {
    cw();
  }
  else
  {
    forward();
  }
}

//----------------------Motor moments------------------------
// The Vex Motor Controller 29 use Servo Control signals to determine speed and direction, with 0 degrees meaning neutral https://en.wikipedia.org/wiki/Servo_control

void disable_motors()
{
  left_font_motor.detach();  // detach the servo on pin left_front to turn Vex Motor Controller 29 Off
  left_rear_motor.detach();  // detach the servo on pin left_rear to turn Vex Motor Controller 29 Off
  right_rear_motor.detach(); // detach the servo on pin right_rear to turn Vex Motor Controller 29 Off
  right_font_motor.detach(); // detach the servo on pin right_front to turn Vex Motor Controller 29 Off

  pinMode(left_front, INPUT);
  pinMode(left_rear, INPUT);
  pinMode(right_rear, INPUT);
  pinMode(right_front, INPUT);
}

void enable_motors()
{
  left_font_motor.attach(left_front);   // attaches the servo on pin left_front to turn Vex Motor Controller 29 On
  left_rear_motor.attach(left_rear);    // attaches the servo on pin left_rear to turn Vex Motor Controller 29 On
  right_rear_motor.attach(right_rear);  // attaches the servo on pin right_rear to turn Vex Motor Controller 29 On
  right_font_motor.attach(right_front); // attaches the servo on pin right_front to turn Vex Motor Controller 29 On
}
void stop() // Stop
{
  left_font_motor.writeMicroseconds(1500);
  left_rear_motor.writeMicroseconds(1500);
  right_rear_motor.writeMicroseconds(1500);
  right_font_motor.writeMicroseconds(1500);
}

void forward()
{
  left_font_motor.writeMicroseconds(1500 + speed_val);
  left_rear_motor.writeMicroseconds(1500 + speed_val);
  right_rear_motor.writeMicroseconds(1500 - speed_val);
  right_font_motor.writeMicroseconds(1500 - speed_val);
}

void reverse()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}

void ccw()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 - speed_val);
  right_font_motor.writeMicroseconds(1500 - speed_val);
}

void cw()
{
  left_font_motor.writeMicroseconds(1500 + speed_val);
  left_rear_motor.writeMicroseconds(1500 + speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}

void strafe_left()
{
  left_font_motor.writeMicroseconds(1500 - speed_val);
  left_rear_motor.writeMicroseconds(1500 + speed_val);
  right_rear_motor.writeMicroseconds(1500 + speed_val);
  right_font_motor.writeMicroseconds(1500 - speed_val);
}

void strafe_right()
{
  left_font_motor.writeMicroseconds(1500 + speed_val);
  left_rear_motor.writeMicroseconds(1500 - speed_val);
  right_rear_motor.writeMicroseconds(1500 - speed_val);
  right_font_motor.writeMicroseconds(1500 + speed_val);
}

void readGyro()
{
  int T = 50;
  gyroRate = (analogRead(sensorPin) * gyroSupplyVoltage) / 1023;
  // find the voltage offset the value of voltage when gyro is zero (still)
  gyroRate -= (gyroZeroVoltage / 1023 * 5);
  // read out voltage divided the gyro sensitivity to calculate the angular velocity
  float angularVelocity = gyroRate / gyroSensitivity;
  // if the angular velocity is less than the threshold, ignore it
  if (angularVelocity >= rotationThreshold || angularVelocity <= -rotationThreshold)
  {
    // we are running a loop in T. one second will run (1000/T).
    float angleChange = angularVelocity / (1000 / T);
    currentAngle += angleChange;
  }
  // keep the angle between 0-360
  if (currentAngle < 0)
  {
    currentAngle += 360;
  }
  else if (currentAngle > 359)
  {
    currentAngle -= 360;
  }
}

void driveStrightWithGyro(int millsecond)
{ 
  readGyro();
  float target =  currentAngle;
  float torlance = 50; // degree
  float directionMinus = 0;
  float directionPlus = 0;
  unsigned long timeAtDestination = millis() + millsecond;

  while (millis() < timeAtDestination)
  {
    readGyro();

    if((abs(currentAngle) < torlance) ||(abs(currentAngle) > (360-torlance)))
    {
      forward();
    }
    else if(currentAngle > torlance)
    {
      cw();
    }
    else
    {
      ccw();
    }

    readGyro();
    delay(200);

    // if((target - torlance)<0){
    //   directionMinus = target - torlance +360;
    // }

    // if(( target + torlance) >360){
    //   directionPlus = target + torlance -360;
    // }

    // SerialCom->println("Current Angle");
    // SerialCom->print(currentAngle);
    // SerialCom->print("Target");   

    // SerialCom->print(target); 
    //     SerialCom->print("directionPlus");
    // SerialCom->print(directionPlus);
    // SerialCom->print("directionMinus");    
    // SerialCom->print(directionMinus);

    
    // if (currentAngle > directionPlus)
    // {
    //   // turn left
    //   SerialCom->println("ccw");
    //   ccw();
    // }
    // else if (currentAngle < directionMinus)
    // {
    //   // turn right
    //   SerialCom->println("cw");
    //   cw();
    // }
    // else
    // {
    //   // go stright
    //   SerialCom->println("forward");
    //   forward();
    // }

    // delay(200);
  }
}
