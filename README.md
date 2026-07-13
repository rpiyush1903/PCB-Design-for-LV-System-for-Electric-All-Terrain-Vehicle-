

# Team AVEON Racing: e-ATV Vehicle Control Unit (VCU)

**Author:** Piyush Ranjan, Vice Captain, Team AVEON Racing

## Project Overview

This repository contains the hardware architecture, schematics, and embedded firmware for the primary Vehicle Control Unit (VCU) developed for an electric All-Terrain Vehicle (e-ATV). Engineered for high-reliability motorsport environments (such as the e-BAJA SAE competition), the VCU strictly manages the low-voltage control systems, actively monitors driver inputs, and interfaces directly with the High Voltage (HV) tractive system to ensure absolute safety compliance.

## System Architecture (Hardware)

The electrical system is strategically designed to isolate the logic control circuits from high-voltage transients and high-current inductive noise sources.

### Power Distribution

* **High Voltage Rail:** The 48V Accumulator routes through a 160A main fuse and an HV Cut-off switch directly to the Motor Controller. (Refer to Aveon Electrical System.jpg)
* **Low Voltage Rail:** A 48V to 12V DC-DC converter steps down the voltage to supply the vehicle's primary 12V accessories and the dashboard safety loop. (Refer to Aveon Electrical System.jpg)
* **Logic Power:** A dedicated 12V to 5V buck converter is implemented to provide an isolated and stable power supply exclusively to the ATmega328P and its corresponding logic circuits. (Refer to Aveon Electrical System.jpg)

### Core Components

* **Microcontroller:** ATmega328P processing unit.
* **Sensor Inputs:** The dashboard cluster feeds inputs including the Ignition Key, Cockpit & Chassis Kill Switches, Brake Pressure Switch, Accelerator (Hall Effect Sensor), and Forward/Neutral/Reverse (FNR) selectors. (Refer to Aveon Electrical System.jpg)
* **Actuator Outputs:** Digital outputs trigger independent relay modules driving the Tractive System Active Light (TSAL), Reverse Light, Main Contact Relay (Accumulator Isolation Relay), Brake Light, and the Ready-To-Drive Sound (RTDS) buzzer. (Refer to Aveon Electrical System.jpg)

## PCB and Schematic Design

The custom printed circuit board layout is engineered to mitigate Electromagnetic Interference (EMI) and prevent CMOS latch-up conditions commonly caused by inductive kickback from automotive relays.

* The schematic incorporates isolated switching pathways and driver circuits for the relay outputs to protect the central ATmega328P processor from voltage spikes.


* Filtering components, alongside structured pull-up/pull-down resistor networks, are utilized on the sensor input lines to guarantee stable state detection in a high-vibration environment.



## Firmware Architecture

The embedded C++ firmware (compiled via Arduino) is built with a focus on continuous safety monitoring, fault prevention, and automated recovery.

### Key Technical Features

1. **Staggered Switching (Power Sag Prevention):** To accommodate the transient response limits of the 12V to 5V buck converter, the firmware enforces a 30-50ms hardware delay between sequential relay activations. This prevents the simultaneous inrush current of multiple relay coils from collapsing the 5V logic rail.
2. **Hardware Watchdog Timer:** Implements a strict 2-second timeout (`WDTO_2S`). Should severe electrical noise halt code execution, the hardware timer automatically triggers a complete system reboot to prevent critical lock-ups.
3. **ADC Auto-Recovery Routine:** Actively re-initializes the Analog-to-Digital Converter enable bit (`ADCSRA |= (1 << ADEN)`) during every main loop iteration. This protects against scenarios where RF interference temporarily shuts down the internal ADC module, ensuring the accelerator pedal is continuously read.
4. **Failsafe Kill Logic:** The safety loop inputs are continuously polled. If the safety circuit opens (detecting 0V), the VCU bypasses all standard logic, immediately de-energizes the primary AIR contactor, and shuts down the tractive system.

## Setup and Deployment

### Prerequisites

* Arduino IDE (v2.0 or higher)
* AVR Board Package installed
* Standard ATmega328P bootloader configuration

### Pin Mapping Directory

| Function | Pin | Type |
| --- | --- | --- |
| **Start Button** | `A0` | Analog Input |
| **Fail-Safe Kill Switch** | `A1` | Analog Input |
| **Accelerator Sensor** | `A2` | Analog Input |
| **Brake Pressure Switch** | `A3` | Analog Input |
| **Reverse Selector** | `A4` | Analog Input |
| **Neutral Selector** | `A5` | Analog Input |
| **TSAL Relay** | `D2` | Digital Output |
| **Reverse Light** | `D3` | Digital Output |
| **Main AIR Contactor** | `D4` | Digital Output |
| **Brake Light** | `D5` | Digital Output |
| **RTDS Buzzer** | `D6` | Digital Output |

### Installation Instructions

1. Clone this repository to your local machine.
2. Open the main `.ino` firmware file in the Arduino IDE.
3. Verify that the pin mappings in the code match your current PCB revision.
4. Connect the VCU via USB and compile/upload the firmware to the ATmega328P.
5. **Pre-flight Check:** Ensure the 12V to 5V power isolation modification is in place before connecting the relay modules to the board.

---

*Developed for Team AVEON Racing. Built for reliability in extreme terrain.*
