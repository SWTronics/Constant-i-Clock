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
void playAlarm();
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
int timerValue = 0;

//button related
int lastButtonState = HIGH; 

unsigned long buttonPressStart = 0;
unsigned long lastButtonPress = 0;
unsigned long lastButtonRepeat = 0;
int stableButtonState = HIGH;
unsigned long lastButtonChange = 0;


unsigned long runButtonPressStart = 0;
bool buttonHeld = false;

const unsigned long DOUBLE_TAP_TIME = 250;   // max time between taps
const unsigned long HOLD_TIME = 500;         // hold starts after 500 ms
const unsigned long REPEAT_START = 200;      // initial hold repeat speed
const unsigned long REPEAT_FAST = 50;        // fastest repeat speed
const unsigned long DEBOUNCE_TIME = 65;
const unsigned long RUN_RESET_HOLD_TIME = 1000;

//State Machine
static system_state_t currentState = STATE_INIT;

// ==========================================================
// PIEZO ALARM
// ==========================================================

const int piezoPin = 7;

const int melody[] = {
    1000, 1200, 1400, 1600,
    1400, 1200, 1000,
    800, 1000, 1200, 1500,
    2000, 1800, 1500, 1200
};

const int melodyDuration[] = {
    150, 150, 150, 250,
    150, 150, 250,
    150, 150, 150, 200,
    300, 150, 150, 400
};

const int melodyLength = sizeof(melody) / sizeof(melody[0]);

int currentNote = 0;
unsigned long lastNoteTime = 0;
bool alarmPlaying = false;

// ==========================================================
// START ANIMATION
// ==========================================================

unsigned long startAnimationStart = 0;
unsigned long lastAnimationStep = 0;

int animationIndex = 0;

// LEDs that make up the "0"
const uint8_t zeroLEDs[] = {
    15, 14, 13, 12,
    8, 9, 10,
    7, 4, 5, 6,
    0, 1, 2
};

const int zeroLEDCount = sizeof(zeroLEDs) / sizeof(zeroLEDs[0]);

const unsigned long START_ANIMATION_TIME = 2000;
const unsigned long ANIMATION_STEP_TIME = 70;

// ============================================================
// main
// ============================================================

void setup(){
  //required by arduino
}
void loop() {

  switch (currentState)
  {

    // ==========================================================
    // TIMER SETTING / INITIALIZATION
    // ==========================================================

    case STATE_INIT:
    {
      pinMode(DATA_PIN, OUTPUT);
      pinMode(SCK_PIN, OUTPUT);
      pinMode(LAT_PIN, OUTPUT);
      pinMode(ENABLE_PIN, OUTPUT);
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      pinMode(piezoPin, OUTPUT);
      noTone(piezoPin);

      digitalWrite(SCK_PIN, LOW);
      digitalWrite(LAT_PIN, LOW);

      // Display OFF while setting timer
      digitalWrite(ENABLE_PIN, HIGH);

      // Reset button tracking
      lastButtonState = HIGH;
      buttonPressStart = 0;
      lastButtonPress = 0;
      lastButtonRepeat = 0;
      buttonHeld = false;

      // Start with timer value of 0
      timerValue = 0;
      timerFinished = false;

      // Show selected timer value
      digitalWrite(ENABLE_PIN, LOW);
      showNumber(timerValue);

      delay(2000);
      currentState = STATE_PRE_RUN;

      break;
    }


    // ==========================================================
    // TIMER SETTING MODE
    // ==========================================================

    case STATE_PRE_RUN:
    {
        unsigned long now = millis();

        int reading = digitalRead(BUTTON_PIN);

        // ==========================================================
        // DEBOUNCE
        // ==========================================================

        if (reading != lastButtonState)
        {
            // Input changed -- restart debounce timer
            lastButtonChange = now;
            lastButtonState = reading;
        }

        // Only accept the new state after it has remained
        // unchanged for the debounce period
        if ((now - lastButtonChange) >= DEBOUNCE_TIME)
        {
            if (reading != stableButtonState)
            {
                stableButtonState = reading;


                // ==================================================
                // NEW PRESS
                // ==================================================

                if (stableButtonState == LOW)
                {
                    // Check for double tap
                   if (lastButtonPress != 0 &&
                    (now - lastButtonPress) <= DOUBLE_TAP_TIME)
                {
                    // DOUBLE TAP -> START ANIMATION

                    currentState = STATE_START_ANIMATION;

                    lastButtonPress = 0;

                    startAnimationStart = 0;
                    animationIndex = 0;
                }
                    else
                    {
                        // FIRST PRESS
                        lastButtonPress = now;

                        // Increment timer
                        timerValue++;

                        if (timerValue > 99)
                            timerValue = 0;

                        showNumber(timerValue);
                    }

                    // Start hold timing
                    buttonPressStart = now;
                    lastButtonRepeat = now;
                }


                // ==================================================
                // BUTTON RELEASED
                // ==================================================

                else
                {
                    // Button released
                }
            }
        }


        // ==========================================================
        // HOLD / AUTO-INCREMENT
        // ==========================================================

        if (stableButtonState == LOW)
        {
            unsigned long heldTime = now - buttonPressStart;

            if (heldTime >= HOLD_TIME)
            {
                unsigned long repeatInterval;

                if (heldTime < 1500)
                {
                    repeatInterval = 200;
                }
                else if (heldTime < 3000)
                {
                    repeatInterval = 100;
                }
                else
                {
                    repeatInterval = 50;
                }

                if (now - lastButtonRepeat >= repeatInterval)
                {
                    lastButtonRepeat = now;

                    timerValue++;

                    if (timerValue > 99)
                        timerValue = 0;

                    showNumber(timerValue);
                }
            }
        }

        break;
    }
    case STATE_START_ANIMATION:
    {
        startAnimation();

        break;
    }
    // ==========================================================
    // TIMER RUNNING
    // ==========================================================
    case STATE_RUN:
    {
        unsigned long now = millis();
        int buttonState = digitalRead(BUTTON_PIN);

        // ==========================================================
        // HOLD BUTTON TO RETURN TO INIT
        // ==========================================================

        if (buttonState == LOW)
        {
            if (runButtonPressStart == 0)
            {
                // Button was just pressed
                runButtonPressStart = now;
            }
            else if (now - runButtonPressStart >= RUN_RESET_HOLD_TIME)
            {
                // Button held long enough -> reset timer

                runButtonPressStart = 0;

                currentState = STATE_INIT;

                break;
            }
        }
        else
        {
            // Button released
            runButtonPressStart = 0;
        }


        // ==========================================================
        // NORMAL COUNTDOWN
        // ==========================================================

        primaryOperation();

        // Check if timer is finished
        if (timerValue == 0)
        {
            currentState = STATE_FINISH;
        }

        break;
    }

    // ==========================================================
    // TIMER FINISHED
    // ==========================================================

    case STATE_FINISH:
    {
      int currentButtonState = digitalRead(BUTTON_PIN);

      playAlarm();
      // Detect new button press
      if (lastButtonState == HIGH && currentButtonState == LOW)
      {
        currentState = STATE_INIT;

        lastButtonState = currentButtonState;
      }
      else
      {
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

  delayMicroseconds(10);

  digitalWrite(LAT_PIN, HIGH);

  delayMicroseconds(10);

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

void playAlarm() {

    unsigned long now = millis();

    // Start first note
    if (!alarmPlaying)
    {
        tone(piezoPin, melody[currentNote]);
        lastNoteTime = now;
        alarmPlaying = true;
    }

    // Move to next note
    if (now - lastNoteTime >= melodyDuration[currentNote])
    {
        lastNoteTime = now;

        currentNote++;

        // Restart melody when finished
        if (currentNote >= melodyLength)
        {
            currentNote = 0;
        }

        tone(piezoPin, melody[currentNote]);
    }
}

void startAnimation()
{
    unsigned long now = millis();

    // Start animation
    if (startAnimationStart == 0)
    {
        startAnimationStart = now;
        lastAnimationStep = now;
        animationIndex = 0;
    }

    // ----------------------------------------------------------
    // Animation finished
    // ----------------------------------------------------------

    if (now - startAnimationStart >= START_ANIMATION_TIME)
    {
        // Show selected timer value
        showNumber(timerValue);

        // Start timer
        timerFinished = false;
        lastTimerUpdate = now;
        timerInterval = getTimerInterval();

        // Reset animation variables
        startAnimationStart = 0;
        animationIndex = 0;

        currentState = STATE_RUN;

        return;
    }

    // ----------------------------------------------------------
    // Advance animation
    // ----------------------------------------------------------

    if (now - lastAnimationStep >= ANIMATION_STEP_TIME)
    {
        lastAnimationStep += ANIMATION_STEP_TIME;

        uint16_t pattern = (1 << zeroLEDs[animationIndex]);

        // Same LED on both digits
        showTwoDigits(pattern, pattern);

        animationIndex++;

        if (animationIndex >= zeroLEDCount)
        {
            animationIndex = 0;
        }
    }
}
