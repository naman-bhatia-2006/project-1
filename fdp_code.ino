// --- Pin Configuration ---
int LeftSensor_Trig = A5;
int LeftSensor_Echo = A4;
int RightSensor_Trig = 11;
int RightSensor_Echo = 12;
int FrontSensor_Trig = A3;
int FrontSensor_Echo = A1;
int RedLED = 10;
int GreenLED = 8;

int Enable1Left = 3;
int Enable2Right = 9;
int LeftMotor_Terminal2 = 2;
int LeftMotor_Terminal1 = 4;
int RightMotor_Terminal2 = 6;
int RightMotor_Terminal1 = 5;

// --- Variables ---
int initialSideDistance = 0;
bool concaveDetected = false;
String ShapeLocation = "";
int FrontDistanceInitially;
float Kp = 3.0, Ki = 0, Kd = 1.2;
float errorSum = 0;
float previousError = 0;
float integralMax = 50; // Anti-windup limit
int shapeConfirmCounter = 0;
const int confirmThreshold = 3;
const int filterSamples = 5;
const int baseSpeed = 150;
const int turnSpeedBase = 150;   // Base speed for turning
const int turnSpeedDiffMax = 100;    // Max speed difference for sharp turns
int sideThreshold = 5; // Threshold for significant change to avoid noise

void setup() {
  Serial.begin(9600);

  // Set pin modes
  pinMode(LeftSensor_Trig, OUTPUT); pinMode(LeftSensor_Echo, INPUT);
  pinMode(RightSensor_Trig, OUTPUT); pinMode(RightSensor_Echo, INPUT);
  pinMode(FrontSensor_Trig, OUTPUT); pinMode(FrontSensor_Echo, INPUT);
  pinMode(Enable1Left, OUTPUT); pinMode(Enable2Right, OUTPUT);
  pinMode(LeftMotor_Terminal1, OUTPUT); pinMode(LeftMotor_Terminal2, OUTPUT);
  pinMode(RightMotor_Terminal1, OUTPUT); pinMode(RightMotor_Terminal2, OUTPUT);
  pinMode(RedLED, OUTPUT); pinMode(GreenLED, OUTPUT);

  digitalWrite(Enable1Left, HIGH);
  digitalWrite(Enable2Right, HIGH);
  digitalWrite(GreenLED, HIGH);
  stopMotors();
  delay(500);

  FrontDistanceInitially = getFilteredDistance(FrontSensor_Trig, FrontSensor_Echo);
  int LeftDistanceInitially = getFilteredDistance(LeftSensor_Trig, LeftSensor_Echo);
  int RightDistanceInitially = getFilteredDistance(RightSensor_Trig, RightSensor_Echo);

  if (RightDistanceInitially < LeftDistanceInitially) {
    initialSideDistance = RightDistanceInitially;
    Serial.println("Shape is in Right Side");
    ShapeLocation = "Right";
  } else {
    initialSideDistance = LeftDistanceInitially;
    Serial.println("Shape is in Left Side");
    ShapeLocation = "Left";
  }
}

void loop() {
  int LeftDistance = getFilteredDistance(LeftSensor_Trig, LeftSensor_Echo);
  int RightDistance = getFilteredDistance(RightSensor_Trig, RightSensor_Echo);
  int FrontDistance = getFilteredDistance(FrontSensor_Trig, FrontSensor_Echo);

  Serial.print("L: "); Serial.print(LeftDistance);
  Serial.print(" R: "); Serial.print(RightDistance);
  Serial.print(" F: "); Serial.println(FrontDistance);

  float error = 0;
  if (ShapeLocation == "Left") {
    error = initialSideDistance - LeftDistance;
  } else {
    error = RightDistance - initialSideDistance;
  }

  bool significantSideChange = abs(error) > sideThreshold;
  bool frontObstacleClose = FrontDistance < FrontDistanceInitially - sideThreshold;

  if (ShapeLocation == "Left") {
    if (significantSideChange && LeftDistance > initialSideDistance + sideThreshold) {
      concaveDetected = false;
      shapeConfirmCounter = 0;
      smoothTurnLeft(abs(LeftDistance - initialSideDistance));
    } 
    else if (significantSideChange && LeftDistance < initialSideDistance - sideThreshold) {
      concaveDetected = true;
      shapeConfirmCounter++;
      smoothTurnRight(abs(LeftDistance - initialSideDistance));
    } 
    else if (frontObstacleClose) {
      concaveDetected = true;
      shapeConfirmCounter++;
      stopMotors();
      digitalWrite(GreenLED, LOW);
      digitalWrite(RedLED, HIGH);
      smoothTurnRight(turnSpeedDiffMax);
      while (true) {
        stopMotors();
      }

    }
    else {
      concaveDetected = false;
      shapeConfirmCounter = 0;
      float correction = pidCorrection(error);
      moveForwardPID(correction);
    }
  } 
  else if (ShapeLocation == "Right") {
    if (significantSideChange && RightDistance > initialSideDistance + sideThreshold) {
      concaveDetected = false;
      shapeConfirmCounter = 0;
      smoothTurnRight(abs(RightDistance - initialSideDistance));
    } 
    else if (significantSideChange && RightDistance < initialSideDistance - sideThreshold) {
      concaveDetected = true;
      shapeConfirmCounter++;
      smoothTurnLeft(abs(RightDistance - initialSideDistance));
    } 
    else if (frontObstacleClose) {
      concaveDetected = true;
      stopMotors();
      digitalWrite(GreenLED, LOW);
      digitalWrite(RedLED, HIGH);

      shapeConfirmCounter++;
      smoothTurnLeft(turnSpeedDiffMax);
      while (true) {
        stopMotors();
      }
    }
    else {
      concaveDetected = false;
      shapeConfirmCounter = 0;
      float correction = pidCorrection(error);
      moveForwardPID(correction);
    }
  }

  if (concaveDetected && shapeConfirmCounter >= confirmThreshold) {
    Serial.println("Concave Shape Confirmed");
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, HIGH);
  } else {
    Serial.println("Convex Shape Likely");
    digitalWrite(GreenLED, HIGH);
    digitalWrite(RedLED, LOW);
  }

  delay(200);
}

// --- PID calculation helper ---
float pidCorrection(float error) {
  errorSum += error;
  errorSum = constrain(errorSum, -integralMax, integralMax);
  float dError = error - previousError;
  float correction = Kp * error + Ki * errorSum + Kd * dError;
  previousError = error;
  return correction;
}

// --- Filtered Sensor Reading (Median Filter) ---
int getFilteredDistance(int trigPin, int echoPin) {
  int readings[filterSamples];
  for (int i = 0; i < filterSamples; i++) {
    readings[i] = getDistance(trigPin, echoPin);
    delay(5);
  }
  for (int i = 0; i < filterSamples - 1; i++) {
    for (int j = 0; j < filterSamples - i - 1; j++) {
      if (readings[j] > readings[j + 1]) {
        int temp = readings[j]; readings[j] = readings[j + 1]; readings[j + 1] = temp;
      }
    }
  }
  return readings[filterSamples / 2];
}

int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 400;
  return duration * 0.034 / 2;
}

// --- Movement Functions ---
void moveForwardPID(float correction) {
  int leftSpeed = baseSpeed - correction;
  int rightSpeed = baseSpeed + correction;
  leftSpeed = constrain(leftSpeed, 100, 255);
  rightSpeed = constrain(rightSpeed, 100, 255);

  analogWrite(Enable1Left, leftSpeed);
  analogWrite(Enable2Right, rightSpeed);

  digitalWrite(LeftMotor_Terminal1, LOW);
  digitalWrite(LeftMotor_Terminal2, HIGH);
  digitalWrite(RightMotor_Terminal1, LOW);
  digitalWrite(RightMotor_Terminal2, HIGH);
}

void smoothTurnLeft(float distanceChange) {
  int speedDiff = map(constrain((int)distanceChange, 0, 50), 0, 50, 0, turnSpeedDiffMax);
  int leftMotorSpeed = turnSpeedBase - speedDiff;
  int rightMotorSpeed = turnSpeedBase;

  leftMotorSpeed = constrain(leftMotorSpeed, 0, 255);

  analogWrite(Enable1Left, leftMotorSpeed);
  analogWrite(Enable2Right, rightMotorSpeed);

  digitalWrite(LeftMotor_Terminal1, LOW);
  digitalWrite(LeftMotor_Terminal2, HIGH);
  digitalWrite(RightMotor_Terminal1, LOW);
  digitalWrite(RightMotor_Terminal2, HIGH);
}

void smoothTurnRight(float distanceChange) {
  int speedDiff = map(constrain((int)distanceChange, 0, 50), 0, 50, 0, turnSpeedDiffMax);
  int rightMotorSpeed = turnSpeedBase - speedDiff;
  int leftMotorSpeed = turnSpeedBase;

  rightMotorSpeed = constrain(rightMotorSpeed, 0, 255);

  analogWrite(Enable1Left, leftMotorSpeed);
  analogWrite(Enable2Right, rightMotorSpeed);

  digitalWrite(LeftMotor_Terminal1, LOW);
  digitalWrite(LeftMotor_Terminal2, HIGH);
  digitalWrite(RightMotor_Terminal1, LOW);
  digitalWrite(RightMotor_Terminal2, HIGH);
}

void stopMotors() {
  digitalWrite(LeftMotor_Terminal1, LOW);
  digitalWrite(LeftMotor_Terminal2, LOW);
  digitalWrite(RightMotor_Terminal1, LOW);
  digitalWrite(RightMotor_Terminal2, LOW);
}
