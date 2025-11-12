// Two-button sketch for Arduino Uno (D2 and D3) using interrupts + debounce
// Remove any other sketches in other tabs that define setup()/loop()

volatile bool btn2Flag = false;
volatile bool btn3Flag = false;

const unsigned long DEBOUNCE_MS = 50;
unsigned long lastDebounce2 = 0;
unsigned long lastDebounce3 = 0;

void handleBtn2_ISR() {
  // Keep ISR tiny: just set a flag
  btn2Flag = true;
}

void handleBtn3_ISR() {
  btn3Flag = true;
}

void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP); // D2 (INT0)
  pinMode(3, INPUT_PULLUP); // D3 (INT1)

  // Attach interrupts (UNO: pin2 -> INT0, pin3 -> INT1)
  // Trigger on FALLING because INPUT_PULLUP means button connects to GND
  attachInterrupt(digitalPinToInterrupt(2), handleBtn2_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), handleBtn3_ISR, FALLING);

  Serial.println("Ready");
}

void loop() {
  unsigned long now = millis();

  if (btn2Flag) {
    // Debounce in main loop
    if (now - lastDebounce2 > DEBOUNCE_MS) {
      lastDebounce2 = now;
      // Double-check the pin state to avoid false triggers
      if (digitalRead(2) == LOW) {
        Serial.println("Button on D2 pressed");
        // <-- put your D2-handling code here
      }
    }
    // clear the flag (either way)
    btn2Flag = false;
  }

  if (btn3Flag) {
    if (now - lastDebounce3 > DEBOUNCE_MS) {
      lastDebounce3 = now;
      if (digitalRead(3) == LOW) {
        Serial.println("Button on D3 pressed");
        // <-- put your D3-handling code here
      }
    }
    btn3Flag = false;
  }

  // small sleep to lower CPU usage
  delay(5);
}