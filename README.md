# Hybrid Thermal Protection & Control System
**SafeThermal-ESP32** | *Embedded Systems / Machine Protection Prototype*

> **Project Objective:** Design and implement a multi-layered control architecture combining **deterministic digital regulation** (RTOS) with **failsafe analog redundancy**. This project models industrial-grade safety systems found in critical thermal management applications, ensuring stability and equipment protection even in the event of total software failure.

---

## 🛡️ System Philosophy: Defense in Depth
This project demonstrates a tiered approach to reliability, critical for industrial automation and process control systems where equipment safety is paramount.

### Layer 1: Real-Time Digital Control (Software)
The primary control layer runs on an **ESP32** utilizing **FreeRTOS** for asymmetric multiprocessing.
*   **Core 1 (Critical):** Executes a 10Hz PID control loop to maintain target temperature (45.0°C) with anti-windup and derivative filtering.
*   **Core 0 (Auxiliary):** Manages telemetry and visualization (74HC595 Shift Register Dashboard) without blocking the critical control loop.
*   **Software Interlock:** A high-priority latching mechanism trips if $T > 60^\circ C$, requiring a cooldown to $<55^\circ C$ before reset.

### Layer 2: Analog Hardware Interlock (Hardware)
Serving as the "Last Resort" protection, a dedicated analog circuit functions independently of the microcontroller. Modeled in TINA-TI/TSC, this stage ensures the heater is physically disconnected during critical faults.

*   **Topology:** Analog Comparator with Hysteresis.
*   **Sensing:** A secondary NTC Thermistor creates a voltage divider against a reference potentiometer.
*   **Logic:** An Operational Amplifier (Op-Amp) compares real-time temperature against the hardware limit.
    *   **Output Stage:** An NPN Inverter driver controls the Gate of an **IRFZ45 Power MOSFET**.
    *   **Hysteresis (Positive Feedback):** A feedback resistor ($R_6$) connects the MOSFET Gate to the Op-Amp non-inverting input. This introduces hysteresis, preventing "chatter" (rapid oscillation) at the threshold and ensuring a decisive hard-cut of power.
*   **Result:** Even if the ESP32 hangs or crashes while the heater is ON, the analog physics of this circuit will override and cut power when the thermal limit is breached.

---

## 🛠️ Technical Implementation

### Digital Control Loop
Implemented a discrete PID controller to minimize steady-state error:
*   **Sampling Rate:** 100ms (10Hz).
*   **Clamping:** Integral term constrained to `[-255, 255]` to prevent saturation delay.
*   **Concurrency:** Task pinning ensures the UI updates never starve the PID math of CPU cycles.

### Hardware Abstraction
*   **Drivers:** Direct GPIO manipulation for low-latency actuation.
*   **Visuals:** Efficient bit-shifting logic drives a 10-segment LED bar graph, visualizing the thermal gradient in real-time.

---

## 🚀 Build & Deployment

The project utilizes `Make` for reproducible builds and `arduino-cli` for toolchain management.

### Prerequisites
*   Linux/macOS environment (or WSL)
*   `make`
*   `arduino-cli`

### Quick Start
```bash
# 1. Compile the firmware
make compile

# 2. Flash to target device (Auto-detect port)
make run

# 3. View Real-time Telemetry
make monitor
```

## 🖥️ Simulation
Validated using **Wokwi** for hardware-in-the-loop (HIL) style simulation before physical deployment.
*   **Configuration:** `diagram.json`
*   **Logic Analyzer:** Pin 2 (Heater PWM) and Pin 5 (Latch) available for timing analysis.

## 🔗 Context
This system was built to demonstrate proficiency in **reliability engineering**, **embedded C++**, and **mixed-signal design patterns**, directly applicable to safety-critical industrial automation, HVAC control systems, and battery thermal management.
