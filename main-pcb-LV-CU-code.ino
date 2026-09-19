/* * PROJECT: Team AVEON Racing VCU - FINAL HARDWARE ADAPTED
 * * HARDWARE SPECIFIC FIXES:
 * 1. START BUTTON (Pin A0):
 * - Active HIGH (5V = Pressed).
 * - Non-latching (Press once to Latch System ON).
 * 2. TSAL "STROBE" FIX: 
 * - Pulse Relay ON for 1000ms, OFF for 100ms to skip the light's built-in pause.
 * * * STANDARD COMPLIANCE:
 * - Kill Switch: Instant cut (Rule 13).
 * - RTDS: 2s Continuous (Rule C.5.7).
 * - Reverse Alarm: Beeps in Reverse (Rule B.10.4.3).
 */

#include <Arduino.h>

// ================================================================
// 1. PIN CONFIGURATION
// ================================================================

// INPUTS
const int PIN_START_BTN = A0; // Start Button (Active HIGH: 5V = Pressed)
const int PIN_KILL      = A1; // Kill Switch (Active HIGH)
const int PIN_ACCEL     = A2; // Analog Accelerator
const int PIN_BRAKE     = A3; // Brake Switch (Active HIGH)
const int PIN_REVERSE   = A4; // Reverse Switch (Active HIGH)
const int PIN_NEUTRAL   = A5; // Neutral Switch (Active HIGH)

// OUTPUTS (Active LOW: 0V = ON/Relay Closed)
const int PIN_TSAL        = 2; 
const int PIN_REV_LIGHT   = 3; 
const int PIN_CONTACT     = 4; // Main AIR Relay
const int PIN_BRAKE_LIGHT = 5; 
const int PIN_RTDS        = 6; // Buzzer Relay

// ================================================================
// 2. TUNING PARAMETERS
// ================================================================

// Throttle Safety
const int ACCEL_THRESHOLD = 50;   

// --- TSAL "RESET" TIMING (CRITICAL FOR YOUR LIGHT) ---
// Your light blinks for 1 second, then pauses. 
// We keep it ON for 1000ms, then OFF for 100ms to restart it.
const unsigned long TSAL_ACTIVE_MS = 1000; // Duration of the "5 blinks"
const unsigned long TSAL_RESET_MS  = 100;  // Quick power cut to reset cycle

// RTDS: 2 Seconds Continuous (Rule C.5.7)
const unsigned long RTDS_DURATION = 2000; 

// REVERSE ALARM: Beep Speed
const unsigned long REV_BEEP_INTERVAL = 500;

// ================================================================
// 3. GLOBAL VARIABLES
// ================================================================

bool vehicleActive = false;       // System Latch State
bool rtdsActive = false;          
unsigned long rtdsStartTime = 0;  
unsigned long lastRevBeepTime = 0; 

// TSAL Timing Variables
unsigned long lastTsalSwitchTime = 0;
bool tsalRelayState = HIGH; // HIGH = OFF, LOW = ON

int revBeepState = HIGH; 

// ================================================================
// 4. SETUP
// ================================================================
void setup() {
  // Inputs
  pinMode(PIN_KILL, INPUT);
  pinMode(PIN_START_BTN, INPUT); 
  pinMode(PIN_ACCEL, INPUT); 
  pinMode(PIN_BRAKE, INPUT);
  pinMode(PIN_REVERSE, INPUT);
  pinMode(PIN_NEUTRAL, INPUT);

  // Outputs
  pinMode(PIN_BRAKE_LIGHT, OUTPUT);
  pinMode(PIN_TSAL, OUTPUT);
  pinMode(PIN_REV_LIGHT, OUTPUT);
  pinMode(PIN_RTDS, OUTPUT);
  pinMode(PIN_CONTACT, OUTPUT);

  // SAFE INITIAL STATE (All OFF)
  digitalWrite(PIN_CONTACT, HIGH);      
  digitalWrite(PIN_TSAL, HIGH);         
  digitalWrite(PIN_RTDS, HIGH);         
  digitalWrite(PIN_BRAKE_LIGHT, HIGH);  
  digitalWrite(PIN_REV_LIGHT, HIGH);    
  
  Serial.begin(9600);
}

// ================================================================
// 5. MAIN LOOP
// ================================================================
void loop() {
  
  // --- A. READ SENSORS ---
  // Start Button Logic: HIGH = Pressed (Active HIGH 5V)
  bool isStartPressed= (digitalRead(PIN_START_BTN) == HIGH); 
  
  // Other Inputs (Assumed Active HIGH based on context)
  bool isKillPressed = (digitalRead(PIN_KILL) == HIGH); 
  bool isBrakePressed= (digitalRead(PIN_BRAKE) == HIGH);
  bool isNeutral     = (digitalRead(PIN_NEUTRAL) == HIGH);
  bool isReverse     = (digitalRead(PIN_REVERSE) == HIGH);
  
  int accelRaw = analogRead(PIN_ACCEL);
  bool isThrottleZero = (accelRaw < ACCEL_THRESHOLD); 

  // --- B. INDEPENDENT LIGHT LOGIC ---
  
  // Brake Light (Rule 13 Exception: Works even if Killed)
  if (isBrakePressed) digitalWrite(PIN_BRAKE_LIGHT, LOW); 
  else digitalWrite(PIN_BRAKE_LIGHT, HIGH);

  // Reverse Light (Rule 13: Must Turn OFF if Killed)
  if (isReverse && !isKillPressed) digitalWrite(PIN_REV_LIGHT, LOW); 
  else digitalWrite(PIN_REV_LIGHT, HIGH); 

  // --- C. SHARED BUZZER LOGIC ---
  int buzzerSignal = HIGH; 

  // Priority 1: RTDS (Continuous on Start)
  if (rtdsActive && !isKillPressed) {
    if (millis() - rtdsStartTime < RTDS_DURATION) {
      buzzerSignal = LOW; // ON (Continuous)
    } else {
      rtdsActive = false; 
      buzzerSignal = HIGH; 
    }
  } 
  // Priority 2: Reverse Alarm (Beeping)
  else if (isReverse && !isKillPressed) {
    if (millis() - lastRevBeepTime >= REV_BEEP_INTERVAL) {
      lastRevBeepTime = millis();
      revBeepState = (revBeepState == HIGH) ? LOW : HIGH;
    }
    buzzerSignal = revBeepState;
  } 
  else {
    buzzerSignal = HIGH; // Quiet
    revBeepState = HIGH; 
  }
  digitalWrite(PIN_RTDS, buzzerSignal);

  // --- D. SAFETY SHUTDOWN CHECK ---
  // Rule 13: Kill Switch de-energizes Tractive System & TSAL
  if (isKillPressed) {
    vehicleActive = false; // Unlatch system
    
    digitalWrite(PIN_CONTACT, HIGH); // AIR OFF
    digitalWrite(PIN_TSAL, HIGH);    // TSAL OFF
    rtdsActive = false;
  }

  // --- E. STARTING SEQUENCE (Rule 12) ---
  else if (!vehicleActive) {
    // Logic: Neutral + Brake + Zero Throttle + Not Reverse + START BUTTON PRESS
    if (isNeutral && isBrakePressed && isThrottleZero && !isReverse && isStartPressed) {
      
      vehicleActive = true; // LATCH: System stays ON after this
      
      digitalWrite(PIN_CONTACT, LOW); // Energize AIR
      
      // Trigger RTDS
      rtdsActive = true;
      rtdsStartTime = millis();
      
      // Reset TSAL Timer
      lastTsalSwitchTime = millis();
      tsalRelayState = LOW; // Start ON immediately
    }
  }

  // --- F. ACTIVE VEHICLE BEHAVIOR ---
  if (vehicleActive) {
    digitalWrite(PIN_CONTACT, LOW); // Keep AIR ON

    // *** TSAL "RELAY RESET" LOGIC ***
    // This fixes the "1 second pause" issue of your hardware light.
    unsigned long tsalCurrentTime = millis();
    
    if (tsalRelayState == LOW) {
      // CURRENTLY ON: Wait for the 5 blinks to finish (1000ms)
      if (tsalCurrentTime - lastTsalSwitchTime >= TSAL_ACTIVE_MS) {
        lastTsalSwitchTime = tsalCurrentTime;
        tsalRelayState = HIGH; // Turn OFF briefly to reset light
      }
    } else {
      // CURRENTLY OFF: Wait for reset (100ms) then turn back ON
      if (tsalCurrentTime - lastTsalSwitchTime >= TSAL_RESET_MS) {
        lastTsalSwitchTime = tsalCurrentTime;
        tsalRelayState = LOW; // Turn ON to restart blink cycle
      }
    }
    digitalWrite(PIN_TSAL, tsalRelayState);
    
  } else {
    // Standby Mode
    digitalWrite(PIN_CONTACT, HIGH);
    digitalWrite(PIN_TSAL, HIGH);
  }
}