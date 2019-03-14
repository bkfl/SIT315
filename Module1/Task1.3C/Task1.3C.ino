const int pir1Pin = 2;
const int pir2Pin = 3;
const int gLedPin = 12;
const int bLedPin = 13;

void setup() {
  // Configure pins
  pinMode(pir1Pin, INPUT_PULLUP);
  pinMode(pir2Pin, INPUT_PULLUP);
  pinMode(bLedPin, OUTPUT);
  pinMode(gLedPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pir1Pin), pir1_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pir2Pin), pir2_ISR, CHANGE);
  // Open serial port
  Serial.begin(9600);
}

void loop() {
  
}

void pir1_ISR() {
  if (digitalRead(pir1Pin) == HIGH){
    Serial.println("Begin: Motion detected on PIR 1");
    Serial.println("Turning blue LED on...");
    digitalWrite(bLedPin, HIGH);
  }
  else {
    Serial.println("End: Motion detected on PIR 1");
    Serial.println("Turning blue LED off...");
    digitalWrite(bLedPin, LOW);
  }
}

void pir2_ISR() {
  if (digitalRead(pir2Pin) == HIGH){
    Serial.println("Begin: Motion detected on PIR 2");
    Serial.println("Turning green LED on...");
    digitalWrite(gLedPin, HIGH);
  }
  else {
    Serial.println("End: Motion detected on PIR 2");
    Serial.println("Turning green LED off...");
    digitalWrite(gLedPin, LOW);
  }
}
