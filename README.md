# 📡 Cellular Telephony Network — GSM-Based Remote Relay Control System
 
<div align="center">
![QUANTRON](https://img.shields.io/badge/Team-QUANTRON-blue?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-ATmega328P-green?style=for-the-badge)
![GSM](https://img.shields.io/badge/GSM-SIM800L-red?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)
 
**A GSM-based embedded system that remotely controls two relay channels via SMS commands and phone calls using the SIM800L module and ATmega328P microcontroller.**
 
</div>
---
 
## 📋 Table of Contents
 
- [Overview](#-overview)
- [Network Theory](#-network-theory)
- [System Architecture](#-system-architecture)
- [Components](#-components)
- [Circuit Diagram](#-circuit-diagram)
- [Software Design](#-software-design)
- [PCB Design](#-pcb-design)
- [Implementation](#-implementation)
- [SMS Command Reference](#-sms-command-reference)
- [Team Members](#-team-members)
- [Supervisors](#-supervisors)
---
 
## 🌐 Overview
 
This project applies **GSM cellular network theory** to a real embedded control system. The **SIM800L** module acts as a mobile station communicating with nearby base stations. Through this GSM infrastructure, the system:
 
- Receives **SMS commands** to switch electrical loads ON/OFF remotely
- Detects **incoming phone calls** and displays caller ID on the LCD
- Provides a **local keypad menu** for on-device relay control
- Shows real-time status on a **16×2 LCD display**
> Built and tested on a **custom-fabricated PCB** manufactured using the toner-transfer + FeCl₃ etching method.
 
---
 
## 📶 Network Theory
 
The system is grounded in **2G GSM architecture**. The four key network elements used:
 
| Element | Role in This Project |
|---|---|
| **Mobile Station (MS)** | SIM800L + SIM card acting as the mobile device |
| **Base Transceiver Station (BTS)** | Cell tower the SIM800L connects to wirelessly |
| **Base Station Controller (BSC)** | Manages multiple BTSs and radio resources |
| **Mobile Switching Center (MSC)** | Routes calls and SMS through the core network |
 
### GSM Network Flow
 
```
Mobile Phone  ──SMS/Call──▶  BTS ──▶ BSC ──▶ MSC ──▶ BTS ──▶ SIM800L ──▶ ATmega328P ──▶ Relay
```
 
---
 
## 🏗️ System Architecture
 
The system is divided into four subsystems:
 
```
┌─────────────────────────────────────────────────────────────────────┐
│                        ATmega328P (Central Unit)                    │
│  ┌──────────────────┐  ┌──────────────┐  ┌────────────────────────┐│
│  │ UART Comm Handler│  │ SMS Parsing  │  │  Call Handling Logic   ││
│  ├──────────────────┤  ├──────────────┤  ├────────────────────────┤│
│  │ Relay Control    │  │ Keypad Driver│  │  LCD Driver            ││
│  ├──────────────────┤  ├──────────────┤  ├────────────────────────┤│
│  │ Menu Navigation  │  │              │  │                        ││
│  └──────────────────┘  └──────────────┘  └────────────────────────┘│
└───────────┬──────────────────┬───────────────────┬─────────────────┘
            │ UART             │ I2C               │ Digital I/O
    ┌───────▼──────┐   ┌───────▼──────┐   ┌────────▼────────┐
    │   SIM800L    │   │  16x2 LCD    │   │  2x Relay Module│
    │  GSM Module  │   │   Display    │   │  (Loads/Lamps)  │
    └───────┬──────┘   └──────────────┘   └─────────────────┘
            │
    ┌───────▼──────┐   ┌──────────────┐
    │  Microphone  │   │   Speaker    │
    └──────────────┘   └──────────────┘
```
 
**4×4 Keypad** connects to ATmega328P via Digital I/O pins.  
**Power Supply**: LM7805 voltage regulator → 5V DC for MCU & relays; ~4V for SIM800L.
 
---
 
## 🔧 Components
 
| # | Component | Model / Part | Function |
|---|---|---|---|
| 1 | Microcontroller | ATmega328P-PU | Central processing unit |
| 2 | GSM Module | SIM800L | 2G cellular communication (calls + SMS) |
| 3 | Display | 16×2 LCD (I2C) | Status, SMS content, menus |
| 4 | Input | 4×4 Membrane Keypad | User input & number dialing |
| 5 | Relay Module | YCRELAY YC3F-DC5V-SHC ×2 | Switch external electrical loads |
| 6 | Voltage Regulator | LM7805 | Regulate supply to 5V DC |
| 7 | Audio Output | Speaker | Voice call output |
| 8 | Audio Input | Electret Microphone | Voice call input |
| 9 | Crystal Oscillator | 16 MHz XTAL | Clock source for ATmega328P |
 
---
 
## ⚡ Circuit Diagram
 
### Prototype (Arduino Nano — Hardware Testing Phase)
 
The system was first validated on an Arduino Nano breadboard prototype before PCB fabrication.
 
```
Components connected in prototype:
  Arduino Nano ←→ SIM800L  (UART: TX/RX)
  Arduino Nano ←→ 4×4 Keypad  (8 digital pins)
  Arduino Nano ←→ OLED Display  (I2C: SDA/SCL)
  SIM800L ←→ Microphone + Speaker  (audio)
  Power: 18650 Li-ion 3400mAh 3.7V battery
```
 
### Final Circuit (ATmega328P Bare-Metal)
 
```
SIM800L ──UART──▶ ATmega328P (D0/D1)
4×4 Keypad ──▶ ATmega328P (D2–D9)
16×2 LCD ──I2C──▶ ATmega328P (A4/A5)
Relay 1 ◀── ATmega328P (D10)
Relay 2 ◀── ATmega328P (D11)
LM7805: VIN(7–12V) → VOUT(5V) → ATmega328P VCC + Relays
SIM800L powered via transistor switch (~4V from raw supply)
```
 
---
 
## 💻 Software Design
 
The firmware runs as a **cooperative main loop** with four sequential stages per iteration:
 
### Overall Flow
 
```
[START]
   │
   ▼
[INIT] → GPIO, LCD (I2C), GSM (UART), AT Commands, Home Screen
   │
   ▼
[MAIN LOOP] ◀─────────────────────────────────────┐
   │                                               │
   ├──▶ [B] GSM Processing (SIM800L serial read)  │
   │         ├── +CMT  → SMS Command Processing   │
   │         │         ├── R1 ON  → Relay1 HIGH   │
   │         │         ├── R1 OFF → Relay1 LOW    │
   │         │         ├── R2 ON  → Relay2 HIGH   │
   │         │         └── R2 OFF → Relay2 LOW    │
   │         ├── RING  → Extract caller, set flag │
   │         └── +CLIP → Store caller number      │
   │                                               │
   ├──▶ [C] Keypad Input                          │
   │         ├── In Menu? ──Yes──▶ Menu Logic [D] │
   │         └── No → Key? ──A──▶ Enter Menu      │
   │                       ──1──▶ Toggle Relay1   │
   │                       ──2──▶ Toggle Relay2   │
   │                                               │
   ├──▶ [D/E] Menu Navigation                     │
   │         Level 1: Relay Control / Last SMS    │
   │         Level 2: Toggle R1, Toggle R2,       │
   │                  Both ON, Both OFF            │
   │                                               │
   └──▶ [F/G] Display Update (every 1.5s) ────────┘
              ├── In Menu? → Show menu screen
              └── No → Show relay states + call info
```
 
### Keypad Shortcuts (Outside Menu)
 
| Key | Action |
|---|---|
| `1` | Toggle Relay 1 |
| `2` | Toggle Relay 2 |
| `A` | Enter Main Menu |
| `B` | Quick Call (placeholder) |
| `C` | Quick SMS (placeholder) |
 
### GSM Initialization AT Commands
 
```at
AT          → Test communication
AT+CLIP=1   → Enable caller ID notification
AT+CMGF=1   → Set SMS text mode
AT+CNMI=... → Enable new SMS notification
```
 
---
 
## 🖥️ PCB Design
 
The PCB was designed in **EDA software** and manufactured in-house.
 
### Manufacturing Process
 
```
1. Design PCB layout in EDA (Eagle/KiCad)
2. Export: Copper Bottom layer + Silk Top mirror layer
3. Laser-print traces on glossy paper
4. Iron-transfer toner onto copper-clad board
5. Etch in FeCl₃ solution (remove exposed copper)
6. Remove toner with acetone
7. Drill component holes
8. Solder all components
```
 
### PCB Layers
 
| Layer | Purpose |
|---|---|
| **Copper Bottom** | All electrical traces and routing |
| **Silk Top (mirrored)** | Component placement reference |
 
---
 
## 🔨 Implementation
 
### Fabricated PCB
 
After FeCl₃ etching, the copper traces are clearly visible on the yellow copper-clad board. The silk-screen layer provides component placement guides.
 
### Final Assembled Board
 
All through-hole components are soldered onto the custom PCB:
- ATmega328P-PU in DIP-28 IC socket
- SIM800L module with helical antenna
- YCRELAY dual relay module (5V coil)
- LM7805 voltage regulator with decoupling capacitors
- 16 MHz crystal oscillator
- Screw terminal connectors for external loads
---
 
## 📟 SMS Command Reference
 
Send these SMS messages to the SIM card inserted in the SIM800L:
 
| SMS Text | Action |
|---|---|
| `R1 ON` | Turn Relay 1 ON |
| `R1 OFF` | Turn Relay 1 OFF |
| `R2 ON` | Turn Relay 2 ON |
| `R2 OFF` | Turn Relay 2 OFF |
 
> ⚠️ Commands are case-sensitive. The system stores the last sender's number and message for review via the LCD menu.
 
---
 
## 👥 Team Members
 
| # | Name |
|---|---|
| 01 | Hossam Mohammed AbdAlNabi Hussein |
| 02 | AlHussein Ehab Mohammed Mohammed |
| 03 | Mohammed Salah Sayed Mohammed |
| 04 | Youssef Ismail Salama Ismail |
| 05 | Mohammed Eprahim Zakaryea Mohammed |
| 06 | Shams Zakareya Galal ElSayed |
 
---
 
## 🎓 Supervisors
 
- **Dr. Ahmed Fawzy**
- **Dr. Mustafa Sallam**
---
 
## 📁 Repository Structure
 
```
📦 cellular-telephony-network
 ┣ 📂 firmware/          # ATmega328P source code (.c / .ino)
 ┣ 📂 pcb/               # EDA project files (schematic + layout)
 ┣ 📂 docs/              # Project report and presentation
 ┣ 📂 images/            # Circuit diagrams, PCB photos, flowcharts
 ┗ 📄 README.md
```
 
---
 
## 📅 Project Info
 
- **Date:** April 29, 2026
- **Team:** QUANTRON
- **Course:** Cellular Telephony Networks
---
 
<div align="center">
Made with ⚡ by **QUANTRON**
 
</div>
