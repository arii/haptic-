/* How to use a Force sensitive resistor to fade an LED with Arduino
   More info: http://www.ardumotive.com/how-to-use-a-force-sensitive-resistor-en.html
   Dev: Michalis Vasilakis // Date: 22/9/2015 // www.ardumotive.com  */
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);


#define USE_IMU;
//#define MOVING_AVG;
//#define DEBUG;


//Constants:
const int handLed = 3;     //pin 3 has PWM funtion
const int footLed = 4;     //pin 3 has PWM funtion


const int sensorPin1 = A1; //pin A0 to read analog input
const int sensorPin2 = A2;
const int sensorPin3 = A3;

const float ACCEL_THRESH_LOW = 7.0;
const float ACCEL_THRESH_HIGH = 13.0;


const int PUSHUP_THRESH_LOW = 150;
const int PUSHUP_THRESH_HIGH = 275;
bool was_restart;

//Variables:
float value, value1, value2, value3, value4; //save analog value
float hand, heel, toe, acc; //save analog value

 enum Mode {
  SQUAT,
  PUSHUP,
  FREEFORM
};

enum Progress{
  RISE,
  FALL,
  BAD,
  RESTART
};

const float RESET_TIME = 2500;
const int MIN_SQUAT_TIME= 500;
float time_counter;
int squat_exercise_counter;
int pushup_exercise_counter;

const int BAD_TOE = 30;

Mode mode;
Progress progress;


void setup() {

  pinMode(handLed, OUTPUT);  //Set pin 3 as 'output'
  pinMode(footLed, OUTPUT);  //Set pin 3 as 'output'

  Serial.begin(9600);       //Begin serial communication
  Serial.println("Hello there!");
  mode = PUSHUP;
  progress = RESTART;
  squat_exercise_counter=0;
  pushup_exercise_counter=0;
  time_counter = millis();
  analogWrite(footLed, 255); 
  analogWrite(handLed, 255); 


  
  #ifdef MOVING_AVG
  for (int i = 0; i < num_avg; i++){
    avg1[i] = 0;
  }
  head = 0;
  tail = num_avg -1;
  #endif
  #ifdef USE_IMU
  if (!accel.begin())
  {
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while (1);
  }
  #endif


}

void loop() {

  value1 = analogRead(sensorPin1);       //Read and save analog value from potentiometer
  value2 = analogRead(sensorPin2);
  value3 = analogRead(sensorPin3);
  value4 = 0;
  value = map(value, 0, 1023, 0, 255);
  //analogWrite(handLed, 255);          //Send PWM value to led
  //analogWrite(footLed, 255);          //Send PWM value to led

#ifdef USE_IMU
  sensors_event_t event;
  accel.getEvent(&event);
  value4 = sqrt( sq(event.acceleration.x) + sq(event.acceleration.y) + sq(event.acceleration.z)); 
#endif

  Serial.println(String(value1) + "," + String(value2) + "," + String(value3) + ',' + String(value4));
  
}



