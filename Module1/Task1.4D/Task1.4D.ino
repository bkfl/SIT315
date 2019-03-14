// Pins
const int wLed_pin = PB2;
const int rLed_pin = PB3;
const int gLed_pin = PB4;
const int bLed_pin = PB5;
const int pir1_pin = PD2;
const int pir2_pin = PD3;
const int snd_pin = PD4;

// Timers
const uint16_t t1_comp = 15625;

// Variables
int pir1_state = 0;
int pir2_state = 0;
int snd_state = 0;

void setup() {
  // Set led pins as outputs
  DDRB |= (1 << wLed_pin);
  DDRB |= (1 << rLed_pin);
  DDRB |= (1 << gLed_pin);
  DDRB |= (1 << bLed_pin);

  PORTD |= (1 << pir1_pin);
  PORTD |= (1 << pir2_pin);
  PORTD |= (1 << snd_pin);

  // Enable pin change interrupts on inputs
  PCMSK2 |= (1 << PCINT18);
  PCMSK2 |= (1 << PCINT19);
  PCMSK2 |= (1 << PCINT20);

  // Enable pin change 2 interrupts in control register
  PCICR |= (1 << PCIE2);

  // Reset timer1 control register
  TCCR1A = 0;

  // Set timer1 CTC mode
  TCCR1B &= ~(1 << WGM13);
  TCCR1B |= (1 << WGM12);

  // Set timer1 prescaler to 1024
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  // Reset timer1
  TCNT1 = 0;

  // Set timer 1 compare value
  OCR1A = t1_comp;

  // Enable timer1 compare interrupt
  TIMSK1 = (1 << OCIE1A);

  // Enable global interrupts
  sei();

  // Open serial comms
  Serial.begin(9600);
}

void loop() {
  
}

ISR(PCINT2_vect) {
  // Read sensor values
  int pir1 = (PIND & (1 << pir1_pin)) >> pir1_pin;
  int pir2 = (PIND & (1 << pir2_pin)) >> pir2_pin;
  int snd = (PIND & (1 << snd_pin)) >> snd_pin;

  // Check if sensor value has changed
  if (pir1 != pir1_state) {
    pir1_handleChange(pir1);
  }
  if (pir2 != pir2_state) {
    pir2_handleChange(pir2);
  }
  if (snd != snd_state) {
    snd_handleChange(snd);
  }
}

ISR(TIMER1_COMPA_vect) {
  // Toggle white led
  PORTB ^= (1 << wLed_pin);
}

void pir1_handleChange(int state) {
  pir1_state = state;
  if (state) {
    Serial.println("Begin: Motion detected on PIR1.");
    Serial.println("Turning red LED on...");
    PORTB |= (1 << rLed_pin);
  } else {
    Serial.println("End: Motion detected on PIR1.");
    Serial.println("Turning red LED off...");
    PORTB &= ~(1 << rLed_pin);
  }
}

void pir2_handleChange(int state) {
  pir2_state = state;
  if (state) {
    Serial.println("Begin: Motion detected on PIR2.");
    Serial.println("Turning green LED on...");
    PORTB |= (1 << gLed_pin);
  } else {
    Serial.println("End: Motion detected on PIR2.");
    Serial.println("Turning green LED off...");
    PORTB &= ~(1 << gLed_pin);
  }
}

void snd_handleChange(int state) {
  snd_state = state;
  if (state) {
    Serial.println("Begin: Sound detected on sensor.");
    Serial.println("Turning blue LED on...");
    PORTB |= (1 << bLed_pin);
  } else {
    Serial.println("End: Sound detected on sensor.");
    Serial.println("Turning blue LED off...");
    PORTB &= ~(1 << bLed_pin);
  }
}
