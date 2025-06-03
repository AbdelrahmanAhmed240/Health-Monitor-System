# 🚨 Smart Patient Monitoring System


[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?logo=linkedin&logoColor=white&style=flat-square)](https://www.linkedin.com/in/abdelrahman-ahmed-729aba256/)

---

## 📋 Project Overview

The **Smart Patient Monitoring System** is a real-time IoT solution tailored for hospital environments, combining embedded systems and web technologies to enhance healthcare monitoring.

By integrating multiple sensors with an ESP32 microcontroller, this system continuously monitors patient vital signs and environmental factors, sending data via MQTT to a Node.js backend. Medical personnel access and control the system through a secure, intuitive web dashboard—enabling remote patient care and room management.

---

## ✨ Key Features

- **Comprehensive real-time monitoring** of:
  - Heart rate
  - Body temperature (LM35 sensor)
  - Room temperature
  - Gas levels (MQ-7 for CO, MQ-135 for CO₂)
- **Two-way communication** via MQTT allowing:
  - Sensor data publishing
  - Remote commands for room control (AC, fans, alerts)
- **Secure and responsive web dashboard** for visualization and alerts
- **Modular and expandable architecture** for future healthcare features, such as:
  - Medication reminders
  - Emergency signals

---

## 🏗️ System Architecture

### Hardware Components

| Component              | Description                       |
|------------------------|---------------------------------|
| ESP32 Microcontroller   | Central controller and communicator |
| Heart Rate Sensor       | Monitors patient's heart rate    |
| LM35 Temperature Sensor | Measures body temperature        |
| MQ-7 Gas Sensor         | Detects carbon monoxide (CO)     |
| MQ-135 Gas Sensor       | Detects carbon dioxide (CO₂)     |
| I2C LCD Display        | Receives commands from staff     |

### Communication

- **Protocol:** MQTT for lightweight, reliable messaging
- **Topics:**
  - `sensor` — Publishes sensor data
  - `command` — Receives remote control instructions

### Software Stack

| Layer       | Technology                         |
|-------------|----------------------------------|
| Backend     | Node.js, Express, MQTT.js         |
| Frontend    | HTML5, CSS3, JavaScript (ES6+)    |
| MQTT Broker | HiveMQ         |

---

## 🚀 Installation & Setup

### Prerequisites

- [Node.js (v14+)](https://nodejs.org/)
- MQTT broker (e.g., [HiveMQ](https://www.hivemq.com/))
- Arduino IDE or PlatformIO for ESP32 firmware programming

### Backend Setup

```bash
# Clone the repository
git clone https://github.com/your-repo-link.git
cd your-repo-link/backend

# Install dependencies
npm install

# Configure MQTT broker details in `.env` or `config.js`

# Start the backend server
npm start
