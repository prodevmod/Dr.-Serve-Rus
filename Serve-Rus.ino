#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

// --- SYSTEM OBJECTS ---
BluetoothSerial SerialBT;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PIN DEFINITIONS ---
// Rotary Encoder
#define ENCODER_A 34  // CLK pin
#define ENCODER_B 35  // DT pin
// L298 Motor Driver
#define MOTOR_IN1 16
#define MOTOR_IN2 17
#define MOTOR_ENA 18  // PWM Speed Control

// --- HARDWARE TUNING VARIABLES ---
// Adjust TICKS_PER_REV if your motor doesn't land perfectly on the quadrants
const int TICKS_PER_REV = 1200; 
const int MOTOR_SPEED = 160;    // PWM Speed (0-255)
const int TOLERANCE = 10;       // Allowed error margin for braking

// --- GLOBAL STATE ---
volatile long encoderTicks = 0;
String currentTraySlot = "Home (0 Deg)";

// --- INTERRUPT SERVICE ROUTINE ---
// This runs in the background to catch every single encoder pulse
void IRAM_ATTR readEncoder() {
  if (digitalRead(ENCODER_B) > 0) {
    encoderTicks++;
  } else {
    encoderTicks--;
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Initialize Bluetooth
  SerialBT.begin("Serve-Rus-Robot");
  Serial.println("Bluetooth Online: Serve-Rus-Robot");

  // 2. Configure Pins
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  // 3. Attach Hardware Interrupt for the Encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), readEncoder, RISING);

  // 4. Initialize OLED Display (SDA=21, SCL=22 default)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED failed to initialize."));
    for(;;); // Halt if OLED fails
  }
  
  updateOLED("SYSTEM ONLINE", "Ready to Pair", "Waiting for AI...");
}

void loop() {
  // Listen for incoming App Inventor Bluetooth text strings
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim(); // Clean up hidden characters

    // Route command to the correct quadrant
    if (command == "GO_45") {
      currentTraySlot = "1st Section";
      navigateToDegrees(45);
    } 
    else if (command == "GO_135") {
      currentTraySlot = "2nd Section";
      navigateToDegrees(135);
    } 
    else if (command == "GO_225") {
      currentTraySlot = "3rd Section";
      navigateToDegrees(225);
    } 
    else if (command == "GO_315") {
      currentTraySlot = "4th Section";
      navigateToDegrees(315);
    }
    // Safety return to home
    else if (command == "GO_0") {
      currentTraySlot = "Home (0 Deg)";
      navigateToDegrees(0);
    }
  }
  
  yield(); // Keep background ESP32 tasks running smoothly
}

// --- CORE MOVEMENT LOGIC ---
void navigateToDegrees(float targetDegrees) {
  // Convert target degrees to absolute encoder ticks
  long targetTicks = (targetDegrees / 360.0) * TICKS_PER_REV;
  
  updateOLED("DISPATCHING", "Moving Motor...", currentTraySlot);

  // Keep driving motor until we reach the target window
  while (abs(encoderTicks - targetTicks) > TOLERANCE) {
    if (encoderTicks < targetTicks) {
      // Current position is behind target -> Spin Forward
      analogWrite(MOTOR_ENA, MOTOR_SPEED);
      digitalWrite(MOTOR_IN1, HIGH);
      digitalWrite(MOTOR_IN2, LOW);
    } else {
      // Current position overshot target -> Spin Backward
      analogWrite(MOTOR_ENA, MOTOR_SPEED);
      digitalWrite(MOTOR_IN1, LOW);
      digitalWrite(MOTOR_IN2, HIGH);
    }
    yield(); 
  }

  // Target reached -> Hard Electronic Brake
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, 0);

  // Update Display
  updateOLED("DEPLOYED", "Position Held", "Slot: " + currentTraySlot);
}

// --- OLED DASHBOARD RENDERER ---
void updateOLED(String statusLine, String actionLine, String footerLine) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setCursor(0, 0);
  display.print("ROBOT: Serve-Rus");
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  
  // Status Body
  display.setCursor(0, 18);
  display.print("STATE: "); 
  display.println(statusLine);
  
  display.setCursor(0, 32);
  display.print("> "); 
  display.println(actionLine);
  
  // Footer
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(0, 54);
  display.print(footerLine);
  
  display.display();
}