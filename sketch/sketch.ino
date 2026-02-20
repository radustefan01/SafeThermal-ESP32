#include <math.h>
#include <Arduino.h>
#include <esp32-hal.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define NTC_PIN 34
#define INTERLOCK_PIN 19
#define HEATER_PIN 2       // LED on pin 2

#define LATCH_PIN 5
#define CLOCK_PIN 18
#define DATA_PIN 23

volatile bool systemSafe = true;
float currentTemp = 0.0;
float targetTemp = 80.0;        
int heaterPWM = 0;

// PID Variables
float Kp = 15.0, Ki = 0.5, Kd = 2.0;
float integral = 0, previousError = 0;

void IRAM_ATTR triggerInterlock() {
  systemSafe = false; 
  digitalWrite(HEATER_PIN, LOW); // Kill power in microseconds
}

void handleSafetySpin() {
  while (!systemSafe || currentTemp >= 60.0) {
    digitalWrite(HEATER_PIN, LOW);
    systemSafe = false;

    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 500) {
      Serial.printf("!!! INTERLOCK TRIPPED !!! Temp: %.2f°C | STATUS: LOCKED\r\n", currentTemp);
      lastPrint = millis();
    }

    vTaskDelay(1 / portTICK_PERIOD_MS); 
    currentTemp = readTemperature(); 
    if (digitalRead(INTERLOCK_PIN) == HIGH && currentTemp < 55.0) {
      systemSafe = true;
      Serial.println(">>> SAFETY RESTORED: Resuming PID control...");
    }
  }
}

float readTemperature() {
  int rawADC = analogRead(NTC_PIN);
  if (rawADC == 0) return 0;
  // Beta equation for 3950 thermistor on 12-bit ADC
  return 1.0 / (log(1.0 / (4095.0 / rawADC - 1.0)) / 3950.0 + 1.0 / 298.15) - 273.15;
}

void TaskPID(void *pvParameters) {
  for (;;) {
    // RUN THE SMART SPIN: If unsafe, this function "traps" the CPU here
    handleSafetySpin();

    // NORMAL OPERATION: Reached only if systemSafe == true
    currentTemp = readTemperature();
    
    // PID Math
    float error = targetTemp - currentTemp;
    integral += error;
    float derivative = error - previousError;
    float output = (Kp * error) + (Ki * integral) + (Kd * derivative);
    previousError = error;
    heaterPWM = constrain((int)output, 0, 255);

    // Logic: Power the LED (Heater) based on PID but gated by the safety
    // In your specific 'Relay-less' circuit, we use Pin 2 HIGH to enable
    digitalWrite(HEATER_PIN, (heaterPWM > 0) ? HIGH : LOW);

    // Optimized Serial Print
    Serial.printf("Temp: %.2f°C | Target: %.2f°C | Power: %d\n\r", 
                  currentTemp, targetTemp, map(heaterPWM, 0, 255, 0, 100));

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// --- 3. Task 2: Shift Register Dashboard (Runs on Core 0) ---
void TaskDashboard(void *pvParameters) {
  for (;;) {
    byte statusByte = 0;

    if (systemSafe) {
      statusByte |= (1 << 7); // Bit 7: Green "SAFE" indicator ON
     
      // Temperature Bar Graph (Bits 0-4)
      if (currentTemp > 20) statusByte |= (1 << 0);
      if (currentTemp > 30) statusByte |= (1 << 1);
      if (currentTemp > 35) statusByte |= (1 << 2);
      if (currentTemp > 40) statusByte |= (1 << 3);
      if (currentTemp > 45) statusByte |= (1 << 4);

      // LED 5 (The "Heater Active" Indicator)
      if (heaterPWM > 0) {
        statusByte |= (1 << 5);
      }
     
    }
    else {
      statusByte |= (1 << 6); // Bit 6: Red "FAULT" indicator ON
    }

    // Shift data to 74HC595
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, statusByte);
    digitalWrite(LATCH_PIN, HIGH);

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --- 4. Main Setup ---
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // Initialize LED pin
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW); // Start with LED off

  // Init Shift Register Pins
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
 
  // Init Interlock Input
  pinMode(INTERLOCK_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERLOCK_PIN), triggerInterlock, FALLING);

  // Spin up the Dual-Core Tasks
  xTaskCreatePinnedToCore(TaskDashboard, "Dashboard", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskPID, "PID_Loop", 2048, NULL, 2, NULL, 1);
}

void loop() {
  // Empty - FreeRTOS handles everything
}