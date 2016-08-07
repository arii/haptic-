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


const int PUSHUP_THRESH_LOW =100;
const int PUSHUP_THRESH_HIGH =300;
bool was_restart;
bool pushup_was_restart;

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

const float RESET_TIME = 1500;
const int MIN_SQUAT_TIME= 500;
float time_counter;
float pushup_time_counter;

int squat_exercise_counter;
int pushup_exercise_counter;

const int BAD_TOE = 30;

Mode mode;
Progress progress;

Progress pushup_progress;


void setup() {

  pinMode(handLed, OUTPUT);  //Set pin 3 as 'output'
  pinMode(footLed, OUTPUT);  //Set pin 3 as 'output'

  Serial.begin(9600);       //Begin serial communication
  Serial.println("Hello there!");
  mode = SQUAT;
  pushup_progress = RESTART;
  progress = RESTART;

  squat_exercise_counter=0;
  pushup_exercise_counter=0;
  time_counter = millis();
  pushup_time_counter = millis();

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

  hand = value3;
  heel = value2;
  toe = value1;
  acc = value4;

  if (hand > 100 && mode != PUSHUP){
    mode = PUSHUP;
    pushup_progress = BAD;
    //Serial.println("SWITCHING TO PUSHUP");
  }
  if(hand < 4 && mode != SQUAT){
    mode= SQUAT;
    progress=BAD;
    //Serial.println("SWITCHING TO SQUATS");
   }
  switch(mode){
    case SQUAT: 
        analogWrite(handLed, 0);  
      break;
    case PUSHUP:
        analogWrite(footLed, 0); 
        //Serial.println("\t\t\t"+String(hand));
      break;
    case FREEFORM:
      break;
  }


  switch(pushup_progress){
    case RISE:
     
     if ( hand > PUSHUP_THRESH_HIGH){
        pushup_progress = FALL;
        pushup_time_counter = millis();
        pushup_was_restart = false;

        #ifdef DEBUG
        Serial.println("\tFinished one pushup. counter++ Switching to Falling");
        #endif
        }
      break;
      
    case FALL:
      if (hand < PUSHUP_THRESH_LOW){
        if (millis() - pushup_time_counter < MIN_SQUAT_TIME){
          #ifdef DEBUG
          Serial.println("Too fast! Ignoring count");
          #endif
        }else{
          pushup_progress=RISE;
          if (!pushup_was_restart && mode==PUSHUP){
          pushup_exercise_counter++;
          Serial.println("Number of Pushups: "+ String(pushup_exercise_counter) + "\tNumber of Squats: "+ String(squat_exercise_counter));
        }
          pushup_time_counter = millis();

          #ifdef DEBUG
          Serial.println("\tSwitching to rising");
          #endif
        }
      }
      break;
      
    case BAD:
      pushup_time_counter = millis();
      pushup_progress=RESTART;
      analogWrite(handLed, 255);          //Send PWM value to led

      #ifdef DEBUG
        Serial.println("\tBAD HAND CONDITION HEEL TOE.  RESTARTING CLOCK");
      #endif
      break;
      
    case RESTART:
      if (millis()-time_counter > RESET_TIME){
        pushup_progress = RISE;
        pushup_time_counter = millis();
        pushup_was_restart = true;

        analogWrite(handLed, 0);          //Send PWM value to led
        #ifdef DEBUG
        Serial.println("\tRestart ended.  Resume Pushup");

        #endif

      }
      break;
  }
  switch(progress){
    case RISE:
     if(toe < heel){
        progress = BAD;
      }
     if (millis() - time_counter > MIN_SQUAT_TIME && acc < ACCEL_THRESH_LOW && heel < 120){
        progress = FALL;
        if (!was_restart && mode==SQUAT){
          squat_exercise_counter++;
          Serial.println("Number of Pushups: "+ String(pushup_exercise_counter) + "\tNumber of Squats: "+ String(squat_exercise_counter));

        }
         time_counter = millis();
        
        }
      break;
      
    case FALL:
     if(toe < heel){
        progress = BAD;
      }
      
      if (acc > ACCEL_THRESH_HIGH){
        if (millis() - time_counter < MIN_SQUAT_TIME){
          #ifdef DEBUG
          Serial.println("Too fast! Ignoring count");
          #endif
        }else{
          progress=RISE;
          time_counter = millis();
          was_restart = false;

          #ifdef DEBUG
          Serial.println("\tSwitching to rising");
          #endif
        }
      }
      break;
      
    case BAD:
      time_counter = millis();
      progress=RESTART;
      analogWrite(footLed, 255);          //Send PWM value to led
      #ifdef DEBUG
      if (mode==SQUAT){
        Serial.println("\tBAD SQUAT CONDITION HEEL TOE.");
      }
      #endif
      
      break;
      
    case RESTART:
      if (millis()-time_counter > RESET_TIME){
        progress = RISE;
        time_counter = millis();
        was_restart = true;

        analogWrite(footLed, 0);          //Send PWM value to led
        #ifdef DEBUG
        Serial.println("\tRestart ended.  Resume Squat");

        #endif

      }
      break;
  }

    bool bad = (mode == SQUAT) && (progress==BAD || progress==RESTART);
    
    Serial.println(
      String(value1) + "," + 
      String(value2) + "," + 
      String(value3) + ',' + 
      String(value4) + ',' +
      String(squat_exercise_counter) +',' + 
      String(pushup_exercise_counter) + ',' +
      String(bad));

  
     
  }







#ifdef MOVING_AVG
//queue
const int num_avg = 10;
int head,tail;
float avg1 [num_avg];
float sum1;
float push(float avg, val){
  weighted_val = val/float(num_avg);
  sum1 += weighted_val - avg1[tail];
  avg1[head] = weighted_val;
  head++;
  tail++;
  if (head == num_avg){
    head = 0;
  }
  if (tail == num_avg){
    tail = 0;
  }
  return sum1
}
#endif


