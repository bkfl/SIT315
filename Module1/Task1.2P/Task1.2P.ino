#define PIR_PIN 2
#define LED_PIN 13

void setup() {
  // Configure pins
  pinMode(PIR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handleMotionChange, CHANGE);
  // Open serial port
  Serial.begin(9600);
}

void loop() {
  
}

void handleMotionChange() {
  if (digitalRead(PIR_PIN) == HIGH){
    motionOn();
  }
  else {
    motionOff();
  }
}

void motionOn() {
  Serial.println("Begin: Motion Detected");
  Serial.println("Turning LED on...");
  digitalWrite(LED_PIN, HIGH);
}

void motionOff() {
  Serial.println("End: Motion Detected");
  Serial.println("Turning LED off...");
  digitalWrite(LED_PIN, LOW);
}
