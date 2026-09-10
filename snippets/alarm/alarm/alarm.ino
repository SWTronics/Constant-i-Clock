// Define the digital pin attached to the piezo disk
const int piezoPin = 7; 

void setup() {
  // Set the piezo pin as an output
  pinMode(piezoPin, OUTPUT); 
}

void loop() {
  // 1. Play a 1000Hz (1kHz) tone
  tone(piezoPin, 1000); 
  delay(1000); // Wait for 1 second
  
  // 2. Stop the sound
  noTone(piezoPin); 
  delay(500); // Wait for 0.5 seconds
  
  // 3. Play a higher pitch tone (2500Hz) for a set duration (500ms)
  tone(piezoPin, 2500, 500); 
  delay(1000); // Wait before repeating the loop
}
