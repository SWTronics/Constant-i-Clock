#define BUTTON_PIN 10  // GPIO 10 on your C3 SuperMini

int lastButtonState = HIGH; 

void setup() {
  Serial.begin(115200);
  
  // CRITICAL: Give the native USB CDC serial a brief moment to wake up
  delay(2000); 
  
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  
  // This heartbeat print lets you verify the connection is active
  Serial.println("--- ESP32-C3 SuperMini Online! ---");
  Serial.println("Pin 10 initialized. Awaiting switch press...");
}

void loop() {
  int currentButtonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Switch was pressed!");
    delay(300); // Anti-bounce delay
  }

  lastButtonState = currentButtonState;
}
