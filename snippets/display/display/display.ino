// Two daisy-chained STP16CPC26
// Countdown timer
#include "definitions.h"
#include <Arduino.h> // Allows you to use standard Arduino functions like digitalWrite or delay



// ============================================================
//  FUNCTIONS
// ============================================================
void shiftOutBits(uint16_t data);
void latch();
void showTwoDigits(uint16_t tensPattern, uint16_t onesPattern);
void showNumber(int number);
unsigned long getTimerInterval();
void flashZero();
void primaryOperation();

// ============================================================
//  VARIABLES
// ============================================================

//timer
unsigned long lastTimerUpdate = 0;
unsigned long lastFlash = 0;
static volatile bool timerFinished = false;
bool flashState = true;
unsigned long timerInterval;

TimerMode timerMode = SECONDS;

// Starting timer value
int timerValue = 10;

//button related
int lastButtonState = HIGH; 

//State Machine
static system_state_t currentState = STATE_INIT;

// ============================================================
// main
// ============================================================

void setup(){
  //required by arduino
}
void loop(){
         switch (currentState)
        {
            case STATE_INIT:
            {
                //reset variables
                timerFinished = false;
                timerValue = 10;

                pinMode(DATA_PIN, OUTPUT);
                pinMode(SCK_PIN, OUTPUT);
                pinMode(LAT_PIN, OUTPUT);
                pinMode(ENABLE_PIN, OUTPUT);

                digitalWrite(SCK_PIN, LOW);
                digitalWrite(LAT_PIN, LOW);

                // Blank while initializing
                digitalWrite(ENABLE_PIN, HIGH);

                showNumber(timerValue);

                // Enable outputs
                digitalWrite(ENABLE_PIN, LOW);

                //Button Configuration
                pinMode(BUTTON_PIN, INPUT_PULLUP);
                // Start timer NOW
                lastTimerUpdate = millis();

                timerInterval = getTimerInterval();
                currentState = STATE_PRE_RUN;
                // Proceed to system validation
                break;
            }


            case STATE_PRE_RUN:
            {
                // Final hardware configuration before entering runtime mode
                currentState = STATE_RUN;
                // Enter normal operating mode
                break;
            }

            case STATE_RUN:
            {
                primaryOperation();

                //Check if timer is finished
                if (timerValue == 0) { 
                  currentState = STATE_FINISH;
                }
                break;
            }

            case STATE_FINISH:
            {
                int currentButtonState = digitalRead(BUTTON_PIN);

                // Detect a new button press (HIGH -> LOW)
                if (lastButtonState == HIGH && currentButtonState == LOW)
                {
                    //Button was pressed
                    currentState = STATE_INIT;

                    // Reset button state
                    lastButtonState = currentButtonState;
                }
                else
                {
                    // Button has NOT been newly pressed
                    // Keep flashing 00
                    flashZero();

                    lastButtonState = currentButtonState;
                }

                break;
            }


            default:
            {
                currentState = STATE_INIT;
                break;
            }
        }
    }



void primaryOperation() {

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
  }
}


void shiftOutBits(uint16_t data) {

  for (int i = 15; i >= 0; i--) {

    digitalWrite(SCK_PIN, LOW);

    digitalWrite(DATA_PIN, (data >> i) & 0x01);

    delayMicroseconds(2);

    digitalWrite(SCK_PIN, HIGH);

    delayMicroseconds(2);
  }

  digitalWrite(SCK_PIN, LOW);
}


void latch() {

  digitalWrite(LAT_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(LAT_PIN, HIGH);

  delayMicroseconds(2);

  digitalWrite(LAT_PIN, LOW);
}


void showTwoDigits(uint16_t tensPattern, uint16_t onesPattern) {

  shiftOutBits(tensPattern);  // sent first -> downstream chip
  shiftOutBits(onesPattern);  // sent second -> upstream chip

  latch();
}



void showNumber(int number) {

  if (number < 0)
    number = 0;

  if (number > 99)
    number = 99;

  int tens = number / 10;
  int ones = number % 10;


  showTwoDigits(DIGITS[ones], DIGITS[tens]);
}



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

void flashZero() {

  static unsigned long lastFlash = 0;
  static bool flashState = false;

  if (millis() - lastFlash >= 500) {

    lastFlash = millis();

    flashState = !flashState;

    if (flashState) {

      // Display 00
      digitalWrite(ENABLE_PIN, LOW);
      showNumber(0);

    } else {

      // Turn display off
      digitalWrite(ENABLE_PIN, HIGH);
    }
  }
}


