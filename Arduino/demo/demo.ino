// include libraries
// call C library in C++ 
#ifdef __cplusplus
#include "Arduino.h"
void setup();
void loop();
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <Servo.h>
#include <stdio.h>
#include <string.h>
#include <DS1302.h>
#include <dht11.h>
#include <Stepper.h>

// define the pins
#define Sensor_AO A0
#define Sensor_DO 8
#define PIN_D A5
#define DHT11PIN 2
#define PIN_SERVO 9
#define STEPS 120
int pinBuzzer = 4;
Stepper stepper(STEPS, 10, 12, 11, 13);

int coverPer = 0; // Storage window coverage status 0(0%) - 100(100%)
int fullCover = 10000;

// objects
Servo myservo;
dht11 DHT11;

unsigned int sensorValue = 0;

namespace {
const int kCePin   = 5;  // Chip Enable
const int kIoPin   = 6;  // Input/Output
const int kSclkPin = 7;  // Serial Clock
/* date variables buffer */
char buf[50];
char day[10];
/* serial port buffer */
String comdata = "";
int numdata[7] = {0}, j = 0, mark = 0;
/* create object DS1302 */
DS1302 rtc(kCePin, kIoPin, kSclkPin);

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

void printTime() {
  // Get the current time and date from the chip.
  Time t = rtc.time();

  // Name the day of the week.
  const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[50];
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);

  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);
}

}  // namespace


void setup() {
  Serial.begin(9600);
  pinMode(Sensor_DO, INPUT);
  pinMode(pinBuzzer, OUTPUT); //Set the Pinbuzzer foot to the output state

  rtc.writeProtect(false);
  rtc.halt(false);
  Time t(2020, 9, 22, 1, 38, 50, Time::kSunday);
  // Set the time and date on the chip.
  rtc.time(t);
 
  stepper.setSpeed(200);

  // Init
  // Rise up Curtain Full Length, Init the Curtain State
  stepper.step((-1)*fullCover);
}

void loop() {
  long frequency = 300; //unit hz
  int val_light;
  myservo.attach(PIN_SERVO);
  //myservo.write(0);

  sensorValue = analogRead(Sensor_AO);
  int chk = DHT11.read(DHT11PIN); //tem
  pinMode(Sensor_DO, INPUT); //smoke
  val_light = analogRead(PIN_D);


  //output
  printTime();
  Serial.print("light status: ");
  Serial.print(val_light);
  if (val_light > 1000)
  {
    Serial.println(" | No light!");
  }
  else
  {
    Serial.println(" | Light!");
  }

  Serial.print("Humidity (%): ");
  Serial.println((float)DHT11.humidity, 2);
  Serial.print("Temperature (oC): ");
  Serial.println((float)DHT11.temperature, 2);
  Serial.print("Sensor AD Value = ");
  Serial.println(sensorValue);
  Serial.println();

  //Check fire
  if (digitalRead(Sensor_DO) == LOW || (float)DHT11.temperature > 50.00)
  {
    Serial.println("Alarm!");
    myservo.write(180);
    tone(pinBuzzer, frequency);
    delay(1000);
    noTone(pinBuzzer);
  }
  
  //Check rain
  if ((float)DHT11.humidity > 80)
  {
    myservo.write(180);
  }
  
  //Check whether dark
  if (val_light > 1000 && coverPer < 100)
  {
    if(coverPer <= 75){
      stepper.step(0.25*fullCover);
      coverPer = coverPer + 25;
    }else{
      stepper.step(fullCover*(1-0.01*coverPer));
      coverPer = 100;
    }
  }

  // Check whether light
  else if(val_light < 21){
    if(coverPer >= 25){
      stepper.step(-0.25*fullCover);
      coverPer = coverPer - 25;
    }else{
      stepper.step(fullCover*-0.01*coverPer);
      coverPer = 0;
    }
  }
  delay(800);
}
