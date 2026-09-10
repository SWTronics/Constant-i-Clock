// Two daisy-chained STP16CPC26
// Countdown timer


const int datPin = 0;  // SDI (to first/upstream chip)
const int sckPin = 1;  // SCK (shared)
const int latPin = 3;  // LAT (shared)
const int enPin  = 4;  // EN (shared, active LOW)


// ============================================================
// TIMER CONFIGURATION
// ============================================================

// Choose one:
// SECONDS
// MINUTES
// HOURS

enum TimerMode {
  SECONDS,
  MINUTES,
  HOURS
};

TimerMode timerMode = MINUTES;

// Starting timer value
int timerValue = 5;


// ============================================================
// DIGIT PATTERNS
// ============================================================

const uint16_t digits[10] = {
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<8)|(1<<9)|(1<<10)|(1<<7)|(1<<4)|(1<<5)|(1<<6)|(1<<0)|(1<<1)|(1<<2), // 0
  (1<<14)|(1<<13)|(1<<12)|(1<<8)|(1<<9)|(1<<10), // 1
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<3)|(1<<11)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<0)|(1<<9), // 2
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<3)|(1<<11)|(1<<8)|(1<<9)|(1<<10)|(1<<7)|(1<<0)|(1<<6), // 3
  (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<11)|(1<<14)|(1<<13)|(1<<12)|(1<<8)|(1<<9)|(1<<10), // 4
  (1<<15)|(1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<11)|(1<<8)|(1<<9)|(1<<10)|(1<<7)|(1<<14)|(1<<6), // 5
  (1<<15)|(1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<11)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<14), // 6
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<8)|(1<<9)|(1<<10)|(1<<0)|(1<<11), // 7
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<8)|(1<<9)|(1<<10)|(1<<7)|(1<<4)|(1<<5)|(1<<6)|(1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<11), // 8
  (1<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<3)|(1<<11)|(1<<0)|(1<<1)|(1<<2)|(1<<8)|(1<<9)|(1<<10)|(1<<7) // 9
};


// ============================================================
// ORIGINAL DISPLAY CODE
// ============================================================

void shiftOutBits(uint16_t data) {

  for (int i = 15; i >= 0; i--) {

    digitalWrite(sckPin, LOW);

    digitalWrite(datPin, (data >> i) & 0x01);

    delayMicroseconds(2);

    digitalWrite(sckPin, HIGH);

    delayMicroseconds(2);
  }

  digitalWrite(sckPin, LOW);
}


void latch() {

  digitalWrite(latPin, LOW);

  delayMicroseconds(2);

  digitalWrite(latPin, HIGH);

  delayMicroseconds(2);

  digitalWrite(latPin, LOW);
}


void showTwoDigits(uint16_t tensPattern, uint16_t onesPattern) {

  shiftOutBits(tensPattern);  // sent first -> downstream chip
  shiftOutBits(onesPattern);  // sent second -> upstream chip

  latch();
}


// ============================================================
// DISPLAY NUMBER
// ============================================================

void showNumber(int number) {

  if (number < 0)
    number = 0;

  if (number > 99)
    number = 99;

  int tens = number / 10;
  int ones = number % 10;


  showTwoDigits(digits[ones], digits[tens]);
}


// ============================================================
// TIMER INTERVAL
// ============================================================

unsigned long timerInterval;

unsigned long getTimerInterval() {

  switch (timerMode) {

    case SECONDS:
      return 1000UL;

    case MINUTES:
      return 60000UL;

    case HOURS:
      return 3600000UL;
  }

  return 1000UL;
}


// ============================================================
// TIMER VARIABLES
// ============================================================

unsigned long lastTimerUpdate = 0;

unsigned long lastFlash = 0;

bool timerFinished = false;
bool flashState = true;

// ============================================================
// FLASH ZERO
// ============================================================

void flashZero() {

  static unsigned long lastFlash = 0;
  static bool flashState = false;

  if (millis() - lastFlash >= 500) {

    lastFlash = millis();

    flashState = !flashState;

    if (flashState) {

      // Display 00
      digitalWrite(enPin, LOW);
      showNumber(0);

    } else {

      // Turn display off
      digitalWrite(enPin, HIGH);
    }
  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  pinMode(datPin, OUTPUT);
  pinMode(sckPin, OUTPUT);
  pinMode(latPin, OUTPUT);
  pinMode(enPin, OUTPUT);

  digitalWrite(sckPin, LOW);
  digitalWrite(latPin, LOW);

  // Blank while initializing
  digitalWrite(enPin, HIGH);

  showNumber(timerValue);

  // Enable outputs
  digitalWrite(enPin, LOW);

  // Start timer NOW
  lastTimerUpdate = millis();

  timerInterval = getTimerInterval();
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // TIMER HAS FINISHED
  // ----------------------------------------------------------

  if (timerFinished) {

    flashZero();

    return;
  }


  // ----------------------------------------------------------
  // CHECK ELAPSED TIME
  // ----------------------------------------------------------

  unsigned long now = millis();

  if (now - lastTimerUpdate >= timerInterval) {

    // Advance based on the scheduled time,
    // not on how long the loop took.
    lastTimerUpdate += timerInterval;

    if (timerValue > 0) {

      timerValue--;

      showNumber(timerValue);
    }


    // --------------------------------------------------------
    // TIMER REACHED ZERO
    // --------------------------------------------------------

    if (timerValue == 0) {

      timerFinished = true;

      // Immediately show 00
      showNumber(0);

      // Begin flashing
      lastFlash = millis();
    }
  }
}