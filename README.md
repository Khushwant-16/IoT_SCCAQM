# IoT_SCCAQM  
## Smart Cloud-Connected Air Quality Monitor (SCCAQM)

---

## 📌 Overview

**SCCAQM** is an advanced IoT ecosystem designed to solve the problem of **environmental sensor drift** in low-cost gas sensors. By deploying an **Adaptive Multi-Sensor Fusion Model (AMSFM)**, the system delivers:

- High-precision air quality diagnostics  
- Energy-aware automation  
- Context-aware security logic  
- Real-time cloud synchronization with a live web dashboard  

This solution bridges **Edge AI, IoT sensing, and cloud intelligence** to create a scalable smart-environment monitoring platform.

---

## 🏗️ System Architecture

The system is structured into four logical layers:

- **Sensing Layer** – Environmental and safety sensors  
- **Processing Layer (Edge AI)** – ESP32 performs real-time calibration and decision-making  
- **Cloud Layer** – Firebase Firestore and Cloud Functions for synchronization  
- **Actuation Layer** – Intelligent mechanical and security responses  

![System Architecture](https://github.com/user-attachments/assets/fd81c04a-6a49-4471-89da-3ddb3ccd7adf)

---

## 🚀 Core Features

### 🔹 AMSFM Calibration (Machine Learning)
- Linear Regression model trained on **12.7 million records**
- Neutralizes **temperature and humidity interference** in MQ-series gas sensors
- Enables **accurate CO and VOC detection** using low-cost hardware

### 🔹 Dual-Mode Security Engine
- **Away Mode:** Intrusion detection using vibration (SW-420)
- **Home Mode:** Seismic or shock event detection
- Mode selection synchronized bi-directionally via cloud configuration

### 🔹 Energy-Aware Actuation
- Servo-driven intelligent ventilation system
- Chooses between:
  - **Passive Cooling:** Closing curtains to block heat
  - **Passive Ventilation:** Opening curtains for fresh air circulation

### 🔹 Real-Time Analytics Dashboard
- Live sensor gauges
- Historical time-series charts
- Trend analysis using Chart.js

---

## 🔌 Hardware Prototype

### 📍 Pin Mapping (ESP32 WROOM)

| Component        | ESP32 Pin       | Type     | Function                         |
|------------------|-----------------|----------|----------------------------------|
| DHT22            | GPIO 4          | Digital  | Temperature & Humidity           |
| MQ-7 (CO)        | GPIO 36 (VP)    | Analog   | Carbon Monoxide Sensing          |
| MQ-9 (VOC/CH₄)   | GPIO 39 (VN)    | Analog   | VOC / Methane Detection          |
| Flame Sensor     | GPIO 33         | Digital  | Fire Detection                   |
| SW-420           | GPIO 25         | Digital  | Vibration / Shock Detection      |
| SG90 Servo       | GPIO 32         | PWM      | Window / Curtain Control         |
| I2C LCD          | GPIO 21 / 22    | I2C      | Local Status Display (Optional)  |

![Hardware Prototype](https://github.com/user-attachments/assets/4de4339a-d309-42ee-9db4-ea7451ddaa1b)

---

## 🧠 Software & ML Stack

### Firmware
- **Language:** C++  
- **Framework:** Arduino (ESP32)

### Cloud Backend
- Firebase Firestore  
- Node.js Cloud Functions  

### Frontend
- HTML5  
- Tailwind CSS  
- JavaScript  
- Chart.js  

### Machine Learning
- Python  
- Scikit-learn  
- Pandas (Time-series resampling)

---

## 🤖 Machine Learning: Adaptive Multi-Sensor Fusion Model (AMSFM)

Low-cost MQ sensors suffer from **chemical drift**, where changes in temperature and humidity falsely resemble gas concentration variations.

### Model Highlights
- **Training Data:** 12.7 million sensor records  
- **Preprocessing:** Resampled to 1-minute intervals  
- **Model:** Linear Regression  

### Deployment Strategy
- Optimized model converted into a **lightweight coefficient array**
- Deployed directly on the **ESP32 (Edge AI)**
- Enables **real-time calibration with zero cloud latency**

---

## ⚙️ Installation & Setup

### 1️⃣ Firebase Setup
1. Create a Firebase project
2. Enable **Firestore Database**
3. Enable **Anonymous Authentication**
4. Deploy Cloud Functions from `/functions`
5. Create Firestore collection:
