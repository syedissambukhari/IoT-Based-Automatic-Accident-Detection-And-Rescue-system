#include <Servo.h>          // Servo motor library
#include <NewPing.h>        // Ultrasonic sensor function library
#include <Wire.h>
#include <MPU6050.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>       // GPS library for parsing GPS data

// Motor control pins
const int LeftMotorForward = 7;
const int LeftMotorBackward = 6;
const int RightMotorForward = 5;
const int RightMotorBackward = 4;

// Ultrasonic sensor pins
#define trig_pin A1
#define echo_pin A2
#define maximum_distance 200

boolean goesForward = false;
int distance = 100;
NewPing sonar(trig_pin, echo_pin, maximum_distance); // Ultrasonic sensor function
Servo servo_motor; // Servo motor
MPU6050 mpu; // MPU6050 object

// GSM setup
const String EMERGENCY_PHONE = "+923275131640"; // Emergency phone number
#define GSM_RX 10
#define GSM_TX 11
#define GPS_RX 8 // RX pin for GPS
#define GPS_TX 9 // TX pin for GPS
SoftwareSerial sim900(GSM_RX, GSM_TX); // GSM
SoftwareSerial gpsSerial(GPS_RX, GPS_TX); // GPS Serial

// Buzzer setup
#define BUZZER 12

// MPU6050 variables
int xaxis = 0, yaxis = 0, zaxis = 0;
int deltx = 0, delty = 0, deltz = 0;
int vibration = 10;
int devibrate = 60;
int magnitude = 0;
int sensitivity = 180; // Ensure sensitivity is declared
byte updateflag = 0; // For accident detection
boolean impact_detected = false;

// GPS variables
TinyGPSPlus gps; // GPS object for parsing GPS data
float latitude = 0.0; // Initial value for latitude
float longitude = 0.0; // Initial value for longitude

void setup() {
  Serial.begin(9600); // Initialize Serial communication
  pinMode(RightMotorForward, OUTPUT);
  pinMode(LeftMotorForward, OUTPUT);
  pinMode(LeftMotorBackward, OUTPUT);
  pinMode(RightMotorBackward, OUTPUT);

  servo_motor.attach(3); // Servo pin

  Wire.begin(); // Initialize I2C for MPU6050
  mpu.initialize(); // Initialize MPU6050

  // Initialize MPU6050 variables
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  xaxis = ax;
  yaxis = ay;
  zaxis = az;

  sim900.begin(9600); // Initialize GSM
  gpsSerial.begin(9600); // Initialize GPS Serial
  
  pinMode(BUZZER, OUTPUT); // Initialize buzzer
  
  servo_motor.write(115);
  delay(2000);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);

  // Check if GSM module is ready
  Serial.println("Checking GSM module...");
  sim900.println("AT");
  delay(1000);
  if (sim900.available()) {
    Serial.println("GSM module ready");
  } else {
    Serial.println("GSM module not responding");
  }

  // Check GSM signal quality
  Serial.println("Checking signal quality...");
  sim900.println("AT+CSQ");
  delay(1000);
  while (sim900.available()) {
    Serial.write(sim900.read());
  }
  
  Serial.println("Setup completed");
  Serial.println("MPU6050 is connected to:");
  Serial.println("SDA -> A4");
  Serial.println("SCL -> A5");
}

void loop() {
  int distanceRight = 0;
  int distanceLeft = 0;
  delay(50);

  if (distance <= 35) {
    moveStop();
    delay(300);
    moveBackward();
    delay(400);
    moveStop();
    delay(300);
    distanceRight = lookRight();
    delay(300);
    distanceLeft = lookLeft();
    delay(300);

    if (distance >= distanceLeft) {
      turnRight();
      moveStop();
    } else {
      turnLeft();
      moveStop();
    }
  } else {
    moveForward(); 
  }
  distance = readPing();

  // Read MPU6050 acceleration data
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  Serial.println("MPU6050 Acceleration - ax: " + String(ax) + ", ay: " + String(ay) + ", az: " + String(az));

  deltx = ax - xaxis;
  delty = ay - yaxis;
  deltz = az - zaxis;

  magnitude = sqrt(sq(deltx) + sq(delty) + sq(deltz));
  Serial.println("Magnitude: " + String(magnitude));

  if (magnitude >= sensitivity) {
    updateflag = 1;
    impact_detected = true;
    Serial.println("Impact detected!"); // Serial output for accident detection
    sendAlert(); // Function to send SMS alert and print to Serial Monitor
    digitalWrite(BUZZER, HIGH); // Activate buzzer
    delay(1000); // Keep buzzer on for 1 second
    digitalWrite(BUZZER, LOW); // Deactivate buzzer
  }

  // GPS data parsing
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read()); // Encode GPS data
  }

  if (gps.location.isValid()) {
    latitude = gps.location.lat();
    longitude = gps.location.lng();
    Serial.print("GPS Coordinates - Lat: ");
    Serial.print(latitude, 6); // Print latitude with 6 decimal places for accuracy
    Serial.print(", Long: ");
    Serial.println(longitude, 6); // Print longitude with 6 decimal places for accuracy
  }

  delay(500); // A small delay to avoid overwhelming the Serial Monitor
}

int lookRight() {  
  servo_motor.write(50);
  delay(500);
  int distance = readPing();
  delay(100);
  servo_motor.write(115);
  return distance;
}

int lookLeft() {
  servo_motor.write(170);
  delay(500);
  int distance = readPing();
  delay(100);
  servo_motor.write(115);
  return distance;
}

int readPing() {
  delay(70);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}

void moveStop() {
  digitalWrite(RightMotorForward, LOW);
  digitalWrite(LeftMotorForward, LOW);
  digitalWrite(RightMotorBackward, LOW);
  digitalWrite(LeftMotorBackward, LOW);
}

void moveForward() {
  if (!goesForward) {
    goesForward = true;
    digitalWrite(LeftMotorForward, HIGH);
    digitalWrite(RightMotorForward, HIGH);
    digitalWrite(LeftMotorBackward, LOW);
    digitalWrite(RightMotorBackward, LOW); 
  }
}

void moveBackward() {
  goesForward = false;
  digitalWrite(LeftMotorBackward, HIGH);
  digitalWrite(RightMotorBackward, HIGH);
  digitalWrite(LeftMotorForward, LOW);
  digitalWrite(RightMotorForward, LOW);
}

void turnRight() {
  digitalWrite(LeftMotorForward, HIGH);
  digitalWrite(RightMotorBackward, HIGH);
  digitalWrite(LeftMotorBackward, LOW);
  digitalWrite(RightMotorForward, LOW);
  delay(250);
  digitalWrite(LeftMotorForward, HIGH);
  digitalWrite(RightMotorForward, HIGH);
  digitalWrite(LeftMotorBackward, LOW);
  digitalWrite(RightMotorBackward, LOW);
}

void turnLeft() {
  digitalWrite(LeftMotorBackward, HIGH);
  digitalWrite(RightMotorForward, HIGH);
  digitalWrite(LeftMotorForward, LOW);
  digitalWrite(RightMotorBackward, LOW);
  delay(250);
  digitalWrite(LeftMotorForward, HIGH);
  digitalWrite(RightMotorForward, HIGH);
  digitalWrite(LeftMotorBackward, LOW);
  digitalWrite(RightMotorBackward, LOW);
}

void sendAlert() {
  String sms_data = "Accident Alert!\n";
  sms_data += "Location: http://maps.google.com/maps?q=loc:" + String(latitude, 6) + "," + String(longitude, 6);
  sendSms(sms_data);

  // Print to Serial Monitor
  Serial.println("Accident Alert!");
  Serial.print("GPS Location: ");
  Serial.print(latitude, 6);
  Serial.print(", ");
  Serial.println(longitude, 6);
}

void sendSms(String text) {
  sim900.print("AT+CMGF=1\r");
  delay(1000);
  sim900.print("AT+CMGS=\"" + EMERGENCY_PHONE + "\"\r");
  delay(1000);
  sim900.print(text);
  delay(100);
  sim900.write(0x1A); // End of SMS
  delay(1000); // Wait for the message to be sent
  Serial.println("SMS sent");
}

void parseData(String buff) {
  // Parsing received data from GSM module
  Serial.println("Received data: " + buff);
}
