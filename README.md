# Washing-Machine-Controller
# 🌞 Solar-Powered Washing Machine Controller (Arduino)

##  Overview

This project presents the design and development of an Arduino-based controller for a solar-powered washing machine. The system ensures that the washing machine can complete all its cycles (wash, rinse, and spin) despite fluctuations in solar power.

The solution focuses on **power management**, **embedded systems**, and **sustainable energy usage**, making it suitable for off-grid or low-income environments.

---

##  Objectives

* Develop a controller that enables full washing cycles using solar power
* Monitor and respond to real-time solar energy availability
* Reduce electricity consumption through efficient system design
* Simulate and test washing machine operations under varying power conditions

---

##  System Architecture

### Core Components

* Solar Panel – Main power source
* Battery – Stores energy for low sunlight conditions
* Power Inverter – Converts DC to AC
* Arduino Uno – Main controller
* Power Sensor (INA219) – Measures power availability
* Motor Driver (L298N) – Controls drum motor
* Water Pump – Simulates water flow
* Sensors – Monitor temperature, water level, and cycle timing
* LCD Display – Displays system status
* Relay Module – Controls switching of components

---

##  Washing Machine Cycles

| Cycle | Estimated Power | Description                         |
| ----- | --------------- | ----------------------------------- |
| Wash  | 300–500 W       | Drum rotation with detergent        |
| Rinse | 200–300 W       | Water refill and cleaning           |
| Spin  | 400–800 W       | High-speed rotation to remove water |

---

##  How It Works

The Arduino continuously monitors available solar power using a power sensor. Based on the available energy:

* The system decides whether to start, pause, or continue a cycle
* Components such as the motor and pump are controlled dynamically
* If power drops, the system adjusts operation to prevent failure

This ensures reliable washing performance even with unstable solar input.

---

##  Methodology

1. Analyze washing machine power requirements
2. Study solar panel performance under different conditions
3. Design a control system using Arduino
4. Develop and implement control software
5. Simulate and test system behavior under power fluctuations
6. Evaluate system performance and limitations

---

## Limitations

* Cannot generate power, only manage it
* Prototype is not full-scale
* Limited heating capability due to power constraints
* Arduino I/O limitations restrict scalability

---

##  Impact & Applications

* Reduces electricity costs
* Supports sustainable living
* Useful in off-grid and rural environments
* Demonstrates practical embedded systems application

---


## Future Improvements

* Integrate smarter power prediction algorithms
* Upgrade to higher-capacity controllers
* Improve water heating efficiency
* Scale system for real household use

---

## 👨‍💻 Author

**Gift Bongani Mahlambi**
