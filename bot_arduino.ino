#include <Servo.h>

/* MOTOR DRIVER */

#define ENA 5
#define ENB 3
#define IN1 8
#define IN2 11
#define IN3 12
#define IN4 13

/* RELAY */

#define RELAY_PIN 4

/* RAIN SENSOR */

#define RAIN_PIN 2

/* ULTRASONIC */

#define TRIG_PIN 9
#define ECHO_PIN 10

/* SERVO */

#define SERVO_PIN 6
Servo arm;

/* DISTANCE VARIABLES */

long duration;
float distance;

/* ARM CONTROL */

int servoPos = 130;
int minAngle = 10;
int maxAngle = 170;

float setPoint = 3.0;
float tolerance = 0.3;

/* MOVE DELAY TIMER */

unsigned long dryStartTime = 0;
bool waitingToMove = false;

/* DISEASE CONTROL */

bool diseaseStop = false;
String command = "";

/* SPRAY TIMER */

bool spraying = false;
unsigned long sprayStartTime = 0;

/* ---------------- DISTANCE FUNCTION ---------------- */

float getDistance() {

digitalWrite(TRIG_PIN, LOW);
delayMicroseconds(2);

digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);

duration = pulseIn(ECHO_PIN, HIGH, 20000);

return duration * 0.034 / 2;

}

/* ---------------- MOTOR FUNCTIONS ---------------- */

void moveForward(){

digitalWrite(IN1,HIGH);
digitalWrite(IN2,LOW);

digitalWrite(IN3,HIGH);
digitalWrite(IN4,LOW);

}

void stopMotor(){

digitalWrite(IN1,LOW);
digitalWrite(IN2,LOW);

digitalWrite(IN3,LOW);
digitalWrite(IN4,LOW);

}

/* ---------------- SETUP ---------------- */

void setup(){

Serial.begin(9600);

pinMode(RAIN_PIN,INPUT);

pinMode(ENA,OUTPUT);
pinMode(ENB,OUTPUT);

pinMode(IN1,OUTPUT);
pinMode(IN2,OUTPUT);
pinMode(IN3,OUTPUT);
pinMode(IN4,OUTPUT);

pinMode(TRIG_PIN,OUTPUT);
pinMode(ECHO_PIN,INPUT);

pinMode(RELAY_PIN,OUTPUT);
digitalWrite(RELAY_PIN,LOW);

arm.attach(SERVO_PIN);
arm.write(servoPos);

analogWrite(ENA,60);
analogWrite(ENB,60);

Serial.println("System Started");

}

/* ---------------- LOOP ---------------- */

void loop(){

/* READ COMMAND FROM ESP8266 */

if (Serial.available()) {

command = Serial.readStringUntil('\n');
command.trim();

/* NEW STOPR COMMAND */

if (command == "STOPR") {

diseaseStop = true;

stopMotor();

digitalWrite(RELAY_PIN,HIGH);

spraying = true;
sprayStartTime = millis();

Serial.println("Spraying Started");

}

}

/* STOP SPRAY AFTER 4 SECONDS */

if(spraying && millis() - sprayStartTime >= 4000){

digitalWrite(RELAY_PIN,LOW);

spraying = false;
diseaseStop = false;

Serial.println("Spraying Finished");

}

/* RAIN CHECK */

int rain = digitalRead(RAIN_PIN);

String rainStatus="Dry";

/* STOP CONDITIONS */

if(rain == HIGH){

rainStatus="Rain Detected";

stopMotor();

waitingToMove=false;

}

else if(diseaseStop == true){

  rainStatus="Disease Detected";
  stopMotor();     // keep bot stopped

}


/* IF NO RAIN AND NO DISEASE */

else{

rainStatus="No Rain";

/* start waiting timer */

if(!waitingToMove){

dryStartTime = millis();
waitingToMove = true;

}

/* wait 5 seconds before moving */

if(millis() - dryStartTime >= 5000){

moveForward();

}

}

/* ---------------- ULTRASONIC SERVO CONTROL ---------------- */

distance = getDistance();

/* ignore far objects */

if (distance > 10 || distance == 0) {

delay(100);

}

else{

float error = setPoint - distance;

/* TOO CLOSE */

if (error > tolerance) {

if (servoPos < maxAngle) {

servoPos++;
arm.write(servoPos);

}

}

/* TOO FAR */

else if (error < -tolerance) {

if (servoPos > minAngle) {

servoPos--;
arm.write(servoPos);

}

}

}

/* SEND RAIN STATUS TO ESP8266 */

Serial.println(rainStatus);

delay(200);

}
