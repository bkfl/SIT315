#define PIR_PIN 2
#define LED_PIN 13

bool motion = false;

void setup() {
  // Set pin modes
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  // Open serial port
  Serial.begin(9600);
}

void loop() {
  // Read PIR state and convert to boolean
  bool newMotion = ( HIGH == digitalRead(PIR_PIN) );
  // Check if state changed
  if (motion != newMotion) {
    motion = newMotion;
    // Respond to state change
    if (motion) {
      Serial.println("Begin: Motion Detected");
      Serial.println("Turning LED on...");
      digitalWrite(LED_PIN, HIGH);
    }
    else {
      Serial.println("End: Motion Detected");
      Serial.println("Turning LED off...");
      digitalWrite(LED_PIN, LOW);
    }
  }
  delay(100);
}
